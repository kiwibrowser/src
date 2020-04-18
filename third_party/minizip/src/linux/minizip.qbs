import qbs;

Project {
    name: "minizip"
    references: [
        "libminizip.qbs"
    ]

    Product {
        name: "minizip"
        type: [ "application" ]
        consoleApplication: true

        Depends { name: "cpp" }
        Depends { name: "libminizip" }

        cpp.cFlags: [ "-std=gnu99" ]

        cpp.commonCompilerFlags: [
            "-Wno-unused-parameter",
            "-Wno-unused-function",
            "-Wno-empty-body"
        ]

        Properties {
            condition: qbs.targetOS.contains("linux")
            cpp.includePaths: outer.concat([
                "/usr/include/"
            ])
            cpp.defines: [
                "__USE_LARGEFILE64",
                "_LARGEFILE64_SOURCE",
                "HAVE_AES"
            ]
            cpp.dynamicLibraries: [
                "z"
            ]
        }

        Group {
            name: "sources"
            prefix: "../"
            files: [
                "minizip.c",
                "minishared.c"
            ]
        }

        Group {
            name: "headers"
            prefix: "../"
            files: [
                "minishared.h"
            ]
        }

        Group {
            // Copy produced executable to install root
            fileTagsFilter: "application"
            qbs.install: true
        }
    }
}
