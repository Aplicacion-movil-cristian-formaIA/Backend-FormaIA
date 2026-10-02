import os, glob, re

column_code = '''
inline std::string safe_get_string(const mysqlx::Value& v) {
    if (v.isNull()) return "";
    try { return v.get<std::string>(); }
    catch (...) {
        try {
            auto b = v.get<mysqlx::bytes>();
            return std::string(reinterpret_cast<const char*>(b.begin()), b.length());
        } catch (...) { return ""; }
    }
}

template <typename T>
'''

column_path = 'src/orm/Column.hpp'
with open(column_path, 'r', encoding='utf-8') as f:
    text = f.read()
if 'safe_get_string' not in text:
    text = text.replace('template <typename T>', column_code)
    with open(column_path, 'w', encoding='utf-8') as f:
        f.write(text)

files = glob.glob('src/domain/entities/*.hpp')
for f in files:
    with open(f, 'r', encoding='utf-8') as file:
        content = file.read()
    
    # Remove the old local lambda definition if present
    content = re.sub(r'auto safeGetString = \[\]\(const mysqlx::Value& v\) -> std::string \{.*?\n\s+\};\n', '', content, flags=re.DOTALL)
    
    # Replace the captured safeGetString in [] with []
    content = re.sub(r'\[safeGetString\]', '[]', content)
    
    # Replace v.get<std::string>() with formaia::orm::safe_get_string(v)
    content = content.replace('v.get<std::string>()', 'formaia::orm::safe_get_string(v)')
    
    # Replace safeGetString(v) with formaia::orm::safe_get_string(v)
    content = content.replace('safeGetString(v)', 'formaia::orm::safe_get_string(v)')

    # Add default {} for SolicitudIA meta_extraida if not there
    if 'SolicitudIA' in f:
        content = content.replace('s.meta_extraida_json = formaia::orm::safe_get_string(v);', 's.meta_extraida_json = formaia::orm::safe_get_string(v); if(s.meta_extraida_json.empty()) s.meta_extraida_json = \"{}\";')
        
    with open(f, 'w', encoding='utf-8') as file:
        file.write(content)
