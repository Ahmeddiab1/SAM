#include <iostream>
#include <sstream>
#include <fstream>

#include <ssc/sscapi.h>

#include "export_config.h"
#include "startup_extractor.h"
#include "input_page_extractor.h"
#include "equation_extractor.h"

/// extract into page_variable_per_config
bool extract_scripts_to_pvpc(std::string ui_path, std::string ui_form_name, page_variables_per_config &pg_vars){
    if (pg_vars.page_to_eqn_outputs[ui_form_name].size() > 0){
        std::cout << "Extracting script from "<< ui_form_name << " which already has eqn outputs.\n";
        return false;
    }
    if (pg_vars.page_to_callback_cmods[ui_form_name].size() > 0){
        std::cout << "Extracting script from "<< ui_form_name << " which already has callback cmods.\n";
        return false;
    }

    input_page_extractor ipl;
    if (!ipl.extract(ui_path + ui_form_name + ".txt")){
        std::cout << "Cannot open " + ui_form_name + " file at " + ui_path;
        return false;
    }

    equation_extractor eqn_ext(ui_form_name);
    eqn_ext.parse_script(ipl.get_eqn_script());

    callback_extractor cb_ext(ui_form_name);
    cb_ext.parse_script(ipl.get_callback_script());

    pg_vars.page_to_eqn_outputs[ui_form_name] = eqn_ext.get_output_variables();
    pg_vars.page_to_callback_cmods[ui_form_name] = cb_ext.get_compute_modules();
    return true;
}

int main(int argc, char *argv[]){
    // startup.lk file path should be provided via command line
    const std::string& filename = "/Users/dguittet/SAM-Development/sam/deploy/runtime/startup.lk";

    if (argc < 2){
        std::cout << "startup.lk script file required.\n";
        return 1;
    }

    std::ifstream ifs(filename.c_str());
    if(!ifs.is_open()){
        std::cout << "cannot open file";
        return 1;
    }

    // load file and extract information for each technology-financial configuration
    std::string content = static_cast<std::stringstream const&>(std::stringstream() << ifs.rdbuf()).str();
    std::vector<std::string> errors;

    startup_extractor su_e;
    su_e.load_startup_script(content, &errors);

    // for each configuration, extract the equations and callback scripts per input page
    std::string ui_path =  "../deploy/runtime/ui/";

//    page_variables_per_config pvpc;
//    extract_scripts_to_pvpc(ui_path, "Inverter CEC Coefficient Generator", pvpc);
//    return 1;

    std::unordered_map<std::string, std::vector<page_info>> SAM_config_to_input_pages = su_e.get_config_to_input_pages();

    for (auto it = SAM_config_to_input_pages.begin(); it != SAM_config_to_input_pages.end(); ++it){
        page_variables_per_config pg_vars;
        pg_vars.config_name = it->first;
        std::vector<page_info> page_info_vector = it->second;

        for (size_t page_n = 0; page_n < page_info_vector.size(); page_n++){
            std::string page_name = page_info_vector[page_n].sidebar_title;

            for (size_t c = 0; c < page_info_vector[page_n].common_uiforms.size(); c++){
                std::string ui_form_name = page_info_vector[page_n].common_uiforms[c];

                if (!extract_scripts_to_pvpc(ui_path, ui_form_name, pg_vars)){
                    return 1;
                }
            }
            for (size_t e = 0; e < page_info_vector[page_n].exclusive_uiforms.size(); e++){
                std::string ui_form_name = page_info_vector[page_n].exclusive_uiforms[e];

                std::vector<std::string> eqn_vars, cb_cmods;
                if (!extract_scripts_to_pvpc(ui_path, ui_form_name, pg_vars)){
                    return 1;
                }
            }
        }
        SAM_page_variables.push_back(pg_vars);
    }

    // output all data in python syntax
    std::cout << "\"\"\"\n";
    std::cout << "\tFile generated by export_config.cpp. Do not edit directly.\n";
    std::cout << "\tExports maps for:\n";
    std::cout << "\t\tconfig_to_input\n";
    std::cout << "\t\tconfig_to_modules\n";
    std::cout << "\t\tconfig_to_eqn_variables\n";
    std::cout << "\t\tconfig_to_cb_cmods\n";
    std::cout << "\tSSC Version: " << ssc_version() << "\n";
    time_t now = time(0);
    std::cout << "\tDate: " << ctime(&now) <<"\n";
    std::cout << "\"\"\"\n\n";
    su_e.print_config_to_input();

    std::cout << "\n\n\n";
    su_e.print_config_to_modules();

    std::cout << "# List of Variables that are calculated in equations #\n\n";
    std::cout << "config_to_eqn_variables = {\n";
    for (size_t n = 0; n < SAM_page_variables.size(); n++) {
        print_page_variables_per_config(SAM_page_variables[n], "eqn");
        std::cout << ",\n";
    }
    std::cout << "}\n\n\n";

    std::cout << "# List of Compute Modules that are executed in callbacks #\n\n";
    std::cout << "config_to_cb_cmods = {\n";
    for (size_t n = 0; n < SAM_page_variables.size(); n++) {
        print_page_variables_per_config(SAM_page_variables[n], "cmod");
        std::cout << ",\n";
    }
    std::cout << "}\n\n";
}

