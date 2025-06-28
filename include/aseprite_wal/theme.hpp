#pragma once
#include <string>
#include <map>

void process_theme_xml(const std::string& template_path, const std::map<std::string, std::string>& color_map, const std::string& output_path);