import qbs;

Product {
    name: "libminizip"
    targetName: "minizip"
    type: [ "staticlibrary" ]

    Depends { name: "cpp" }

    cpp.commonCompilerFlags: [
        "-Wno-unused-parameter",
        "-Wno-unused-function",
        "-Wno-empty-body",
        "-Wno-sequence-point"
    ]

    Properties {
        condition: qbs.targetOS.contains("linux")
        cpp.includePaths: outer.concat([
            "/usr/include/"
        ])
        cpp.defines: [
            "__USE_FILE_OFFSET64",
            "__USE_LARGEFILE64",
            "_LARGEFILE64_SOURCE",
            "_FILE_OFFSET_BIT=64",
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
            "crypt.c",
            "ioapi.c",
            "ioapi_buf.c",
            "ioapi_mem.c",
            "unzip.c",
            "zip.c"
        ]
    }

    Group {
        name: "aes sources"
        prefix: "../aes/"
        files: [
            "aescrypt.c",
            "aeskey.c",
            "aes_ni.c",
            "aestab.c",
            "fileenc.c",
            "hmac.c",
            "prng.c",
            "pwd2key.c",
            "sha1.c"
        ]
    }

    Group {
        name: "headers"
        prefix: "../"
        files: [
            "crypt.h",
            "ioapi.h",
            "ioapi_buf.h",
            "ioapi_mem.h",
            "unzip.h",
            "zip.h"
        ]
    }

    Group {
        name: "aes headers"
        prefix: "../aes/"
        files: [
            "aes.h",
            "aes_ni.h",
            "aesopt.h",
            "aestab.h",
            "brg_endian.h",
            "brg_types.h",
            "fileenc.h",
            "hmac.h",
            "prng.h",
            "pwd2key.h",
            "sha1.h"
        ]
    }
}



