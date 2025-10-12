#include "type_converter.hpp"

#include <cctype>

namespace curious::dsl::capnpgen
{

std::string TypeConverter::indent(int level)
{
    return std::string(level * 4, ' ');
}

std::string TypeConverter::to_capnp_method_name(const std::string& field_name)
{
    if (field_name.empty())
    {
        return field_name;
    }

    std::string result = field_name;

    // Capitalize first letter
    result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));

    return result;
}

std::string TypeConverter::generate_from_capnp_code(const Type& field,
                                                      const std::string& reader_expr,
                                                      const std::string& target_var,
                                                      int indent_level)
{
    std::ostringstream code;
    std::string ind = indent(indent_level);

    std::string getter_name = to_capnp_method_name(field.get_field_name());

    if (field.is_primitive())
    {
        // Primitive types: direct assignment
        code << ind << target_var << " = " << reader_expr << ".get" << getter_name << "();\n";
    }
    else if (field.is_custom() || field.is_enum())
    {
        // Custom/Enum types
        std::string type_name = field.get_custom_name();

        code << ind << "if (" << reader_expr << ".has" << getter_name << "())\n";
        code << ind << "{\n";

        if (type_name == "MessageType" || field.is_enum())
        {
            // Enum: static cast
            code << ind << "    " << target_var << " = static_cast<" << type_name
                 << ">(" << reader_expr << ".get" << getter_name << "());\n";
        }
        else
        {
            // Custom message: call from_capnp_struct
            code << ind << "    " << target_var << ".from_capnp_struct("
                 << reader_expr << ".get" << getter_name << "());\n";
        }

        code << ind << "}\n";
    }
    else if (field.is_list())
    {
        const Type* element_type = field.get_element_type();

        code << ind << "if (" << reader_expr << ".has" << getter_name << "())\n";
        code << ind << "{\n";
        code << ind << "    auto list_reader = " << reader_expr << ".get" << getter_name << "();\n";
        code << ind << "    " << target_var << ".clear();\n";
        code << ind << "    " << target_var << ".reserve(list_reader.size());\n";
        code << ind << "    for (const auto& item : list_reader)\n";
        code << ind << "    {\n";

        if (element_type->is_primitive())
        {
            code << ind << "        " << target_var << ".push_back(item);\n";
        }
        else if (element_type->is_custom() || element_type->is_enum())
        {
            std::string elem_type_name = element_type->get_custom_name();
            if (element_type->is_enum())
            {
                code << ind << "        " << target_var << ".push_back(static_cast<"
                     << elem_type_name << ">(item));\n";
            }
            else
            {
                code << ind << "        " << elem_type_name << " elem;\n";
                code << ind << "        elem.from_capnp_struct(item);\n";
                code << ind << "        " << target_var << ".push_back(std::move(elem));\n";
            }
        }

        code << ind << "    }\n";
        code << ind << "}\n";
    }
    else if (field.is_map())
    {
        const Type* key_type = field.get_key_type();
        const Type* value_type = field.get_value_type();

        code << ind << "if (" << reader_expr << ".has" << getter_name << "())\n";
        code << ind << "{\n";
        code << ind << "    auto map_reader = " << reader_expr << ".get" << getter_name << "();\n";
        code << ind << "    " << target_var << ".clear();\n";
        code << ind << "    if (map_reader.hasEntries())\n";
        code << ind << "    {\n";
        code << ind << "        auto entries = map_reader.getEntries();\n";
        code << ind << "        for (const auto& entry : entries)\n";
        code << ind << "        {\n";

        // Read key
        std::string key_read = "entry.getKey()";
        std::string value_read = "entry.getValue()";

        if (key_type->is_primitive())
        {
            if (value_type->is_primitive())
            {
                code << ind << "            " << target_var << "[" << key_read << "] = "
                     << value_read << ";\n";
            }
            else if (value_type->is_custom())
            {
                std::string val_type_name = value_type->get_custom_name();
                code << ind << "            " << val_type_name << " val;\n";
                code << ind << "            val.from_capnp_struct(" << value_read << ");\n";
                code << ind << "            " << target_var << "[" << key_read
                     << "] = std::move(val);\n";
            }
        }

        code << ind << "        }\n";
        code << ind << "    }\n";
        code << ind << "}\n";
    }

    return code.str();
}

std::string TypeConverter::generate_to_capnp_code(const Type& field,
                                                    const std::string& builder_expr,
                                                    const std::string& source_var,
                                                    const std::string& field_name_capnp,
                                                    int indent_level)
{
    std::ostringstream code;
    std::string ind = indent(indent_level);

    std::string setter_name = "set" + to_capnp_method_name(field_name_capnp);
    std::string init_name = "init" + to_capnp_method_name(field_name_capnp);

    if (field.is_primitive())
    {
        // Primitive types: direct set
        code << ind << builder_expr << "." << setter_name << "(" << source_var << ");\n";
    }
    else if (field.is_custom() || field.is_enum())
    {
        std::string type_name = field.get_custom_name();

        if (type_name == "MessageType" || field.is_enum())
        {
            // Enum: static cast
            code << ind << builder_expr << "." << setter_name << "(static_cast<NetworkMsg::"
                 << type_name << ">(" << source_var << "));\n";
        }
        else
        {
            // Custom message: call to_capnp_struct
            code << ind << "{\n";
            code << ind << "    auto nested_builder = " << builder_expr << "."
                 << init_name << "();\n";
            code << ind << "    " << source_var << ".to_capnp_struct(nested_builder);\n";
            code << ind << "}\n";
        }
    }
    else if (field.is_list())
    {
        const Type* element_type = field.get_element_type();

        code << ind << "if (!" << source_var << ".empty())\n";
        code << ind << "{\n";
        code << ind << "    auto list_builder = " << builder_expr << "." << init_name
             << "(" << source_var << ".size());\n";
        code << ind << "    for (size_t i = 0; i < " << source_var << ".size(); ++i)\n";
        code << ind << "    {\n";

        if (element_type->is_primitive())
        {
            code << ind << "        list_builder.set(i, " << source_var << "[i]);\n";
        }
        else if (element_type->is_enum())
        {
            std::string elem_type_name = element_type->get_custom_name();
            code << ind << "        list_builder.set(i, static_cast<NetworkMsg::"
                 << elem_type_name << ">(" << source_var << "[i]));\n";
        }
        else if (element_type->is_custom())
        {
            code << ind << "        auto item_builder = list_builder[i];\n";
            code << ind << "        " << source_var << "[i].to_capnp_struct(item_builder);\n";
        }

        code << ind << "    }\n";
        code << ind << "}\n";
    }
    else if (field.is_map())
    {
        const Type* key_type = field.get_key_type();
        const Type* value_type = field.get_value_type();

        code << ind << "if (!" << source_var << ".empty())\n";
        code << ind << "{\n";
        code << ind << "    auto map_builder = " << builder_expr << "." << init_name << "();\n";
        code << ind << "    auto entries_builder = map_builder.initEntries("
             << source_var << ".size());\n";
        code << ind << "    size_t idx = 0;\n";
        code << ind << "    for (const auto& [key, value] : " << source_var << ")\n";
        code << ind << "    {\n";
        code << ind << "        auto entry_builder = entries_builder[idx++];\n";

        // Set key
        if (key_type->is_primitive())
        {
            code << ind << "        entry_builder.setKey(key);\n";
        }

        // Set value
        if (value_type->is_primitive())
        {
            code << ind << "        entry_builder.setValue(value);\n";
        }
        else if (value_type->is_enum())
        {
            std::string val_type_name = value_type->get_custom_name();
            code << ind << "        entry_builder.setValue(static_cast<NetworkMsg::"
                 << val_type_name << ">(value));\n";
        }
        else if (value_type->is_custom())
        {
            code << ind << "        auto value_builder = entry_builder.initValue();\n";
            code << ind << "        value.to_capnp_struct(value_builder);\n";
        }

        code << ind << "    }\n";
        code << ind << "}\n";
    }

    return code.str();
}

std::string TypeConverter::get_default_value(const Type& field)
{
    if (field.is_primitive())
    {
        std::string cpp_type = field.get_cpp_type();

        if (cpp_type.find("int") != std::string::npos)
        {
            return "0";
        }
        else if (cpp_type == "float" || cpp_type == "double")
        {
            return "0.0";
        }
        else if (cpp_type == "bool")
        {
            return "false";
        }
        else if (cpp_type == "std::string")
        {
            return "\"\"";
        }
        else if (cpp_type.find("vector") != std::string::npos)
        {
            return "{}";
        }
    }
    else if (field.is_list() || field.is_map())
    {
        return "{}";
    }
    else if (field.is_custom())
    {
        return field.get_custom_name() + "{}";
    }
    else if (field.is_enum())
    {
        // Default to first value or 0
        return "static_cast<" + field.get_custom_name() + ">(0)";
    }

    return "{}";
}

} // namespace curious::dsl::capnpgen
