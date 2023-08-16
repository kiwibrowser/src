//* Copyright 2019 The Dawn Authors
//*
//* Licensed under the Apache License, Version 2.0 (the "License");
//* you may not use this file except in compliance with the License.
//* You may obtain a copy of the License at
//*
//*     http://www.apache.org/licenses/LICENSE-2.0
//*
//* Unless required by applicable law or agreed to in writing, software
//* distributed under the License is distributed on an "AS IS" BASIS,
//* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//* See the License for the specific language governing permissions and
//* limitations under the License.

// Command handlers & doers
{% for command in cmd_records["command"] %}
    {% set Suffix = command.name.CamelCase() %}
    bool Handle{{Suffix}}(const volatile char** commands, size_t* size);

    bool Do{{Suffix}}(
        {%- for member in command.members -%}
            {%- if member.is_return_value -%}
                {%- if member.handle_type -%}
                    {{as_cType(member.handle_type.name)}}* {{as_varName(member.name)}}
                {%- else -%}
                    {{as_cType(member.type.name)}}* {{as_varName(member.name)}}
                {%- endif -%}
            {%- else -%}
                {{as_annotated_cType(member)}}
            {%- endif -%}
            {%- if not loop.last -%}, {% endif %}
        {%- endfor -%}
    );
{% endfor %}

{% for CommandName in server_custom_pre_handler_commands %}
    bool PreHandle{{CommandName}}(const {{CommandName}}Cmd& cmd);
{% endfor %}
