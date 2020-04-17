# Copyright 2018 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Rule that allows to embed binary files into a C++ program.

The result is exposed as an absl::string_view global variable.
"""

def cc_data_blob(name, src, varname):
    native.genrule(
        name = name + "_sources",
        srcs = [src],
        outs = [name + ".cc", name + ".h"],
        cmd = "; ".join([
            "cat $< > $(@D)/datablob.tmp",
            "pushd $(@D)",
            "outname=%s" % name,
            "echo 'namespace {' > $$outname.cc",
            "xxd -i datablob.tmp >> $$outname.cc",
            "echo '}' >> $$outname.cc",
            "echo '#include \"absl/strings/string_view.h\"' >> $$outname.cc",
            "echo 'absl::string_view %s(reinterpret_cast<const char*>(datablob_tmp), datablob_tmp_len);' >> $$outname.cc" % varname,
            "echo '#include \"absl/strings/string_view.h\"' >> $$outname.h",
            "echo 'extern absl::string_view %s;' >> $$outname.h" % varname,
            "rm datablob.tmp",
            "popd",
        ]),
    )
    native.cc_library(
        name = name,
        srcs = [name + ".cc"],
        hdrs = [name + ".h"],
        deps = ["@com_google_absl//absl/strings"],
    )
