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

//* Return command handlers
{% for command in cmd_records["return command"] %}
    bool Handle{{command.name.CamelCase()}}(const volatile char** commands, size_t* size);
{% endfor %}

//* Return command doers
{% for command in cmd_records["return command"] %}
    bool Do{{command.name.CamelCase()}}(
        {%- for member in command.members -%}
            {%- if member.handle_type -%}
                {{as_wireType(member.handle_type)}} {{as_varName(member.name)}}
            {%- else -%}
                {{as_annotated_wireType(member)}}
            {%- endif -%}
            {%- if not loop.last -%}, {% endif %}
        {%- endfor -%}
    );
{% endfor %}
