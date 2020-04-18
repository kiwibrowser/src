/*
 * Copyright © 2011 Linaro Limited
 * Copyright © 2011 0xlab - http://0xlab.org/
 *
 * This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
 *
 * glmark2 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * glmark2.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Alexandros Frantzis (glmark2)
 *  Jim Huang (Strict JNI registration using JNI_OnLoad())
 */
#include <assert.h>
#include <jni.h>
#include <vector>
#include <string>
#include <fstream>
#include "canvas-android.h"
#include "benchmark.h"
#include "options.h"
#include "log.h"
#include "util.h"
#include "main-loop.h"
#include "benchmark-collection.h"
#include "scene-collection.h"

static Canvas *g_canvas;
static MainLoop *g_loop;
static BenchmarkCollection *g_benchmark_collection;
static SceneCollection *g_scene_collection;
static std::ostream *g_log_extra;

class MainLoopAndroid : public MainLoop
{
public:
    MainLoopAndroid(Canvas &canvas, const std::vector<Benchmark *> &benchmarks) :
        MainLoop(canvas, benchmarks) {}

    virtual void log_scene_info() {}

    virtual void log_scene_result()
    {
        if (scene_setup_status_ == SceneSetupStatusSuccess) {
            Log::info("%s FPS: %u FrameTime: %.3f ms\n",
                      scene_->info_string().c_str(),
                      scene_->average_fps(),
                      1000.0 / scene_->average_fps());
        }
        else if (scene_setup_status_ == SceneSetupStatusUnsupported) {
            Log::info("%s Unsupported\n",
                      scene_->info_string().c_str());
        }
        else {
            Log::info("%s Set up failed\n",
                      scene_->info_string().c_str());
        }
    }
};

class MainLoopDecorationAndroid : public MainLoopDecoration
{
public:
    MainLoopDecorationAndroid(Canvas &canvas, const std::vector<Benchmark *> &benchmarks) :
        MainLoopDecoration(canvas, benchmarks) {}

    virtual void log_scene_info() {}

    virtual void log_scene_result()
    {
        if (scene_setup_status_ == SceneSetupStatusSuccess) {
            Log::info("%s FPS: %u FrameTime: %.3f ms\n",
                      scene_->info_string().c_str(),
                      scene_->average_fps(),
                      1000.0 / scene_->average_fps());
        }
        else if (scene_setup_status_ == SceneSetupStatusUnsupported) {
            Log::info("%s Unsupported\n",
                      scene_->info_string().c_str());
        }
        else {
            Log::info("%s Set up failed\n",
                      scene_->info_string().c_str());
        }
    }
};

/** 
 * Converts an std::vector containing arguments to argc,argv.
 */
static void
arg_vector_to_argv(const std::vector<std::string> &arguments, int &argc, char **&argv)
{
    argc = arguments.size() + 1;
    argv = new char* [argc];
    argv[0] = strdup("glmark2");

    for (unsigned int i = 0; i < arguments.size(); i++)
        argv[i + 1] = strdup(arguments[i].c_str());
}

/** 
 * Populates the command line arguments from the arguments file.
 * 
 * @param argc the number of arguments
 * @param argv the argument array
 */
static void
get_args_from_file(const std::string &arguments_file, int &argc, char **&argv)
{
    std::vector<std::string> arguments;
    std::ifstream ifs(arguments_file.c_str());

    if (!ifs.fail()) {
        std::string line;
        while (getline(ifs, line)) {
            if (!line.empty())
                Util::split(line, ' ', arguments, Util::SplitModeQuoted);
        }
    }

    arg_vector_to_argv(arguments, argc, argv);
}

/** 
 * Populates the command line arguments from the arguments file.
 * 
 * @param argc the number of arguments
 * @param argv the argument array
 */
static void
get_args_from_string(const std::string &args_str, int &argc, char **&argv)
{
    std::vector<std::string> arguments;
    Util::split(args_str, ' ', arguments, Util::SplitModeQuoted);

    arg_vector_to_argv(arguments, argc, argv);
}

/** 
 * Releases the command line arguments.
 * 
 * @param argc the number of arguments
 * @param argv the argument array
 */
static void
release_args(int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
        free(argv[i]);

    delete[] argv;
}

/** 
 * Converts a GLVisualConfig Java object to a GLVisualConfig C++ object.
 * 
 * @param env the JNIEnv
 * @param jvc the Java VisualConfig object to convert
 * @param vc the C++ VisualConfig object to fill
 */
static void
gl_visual_config_from_jobject(JNIEnv *env, jobject jvc, GLVisualConfig &vc)
{
    jclass cls = env->GetObjectClass(jvc);
    jfieldID fid;

    fid = env->GetFieldID(cls, "red", "I");
    vc.red = env->GetIntField(jvc, fid);

    fid = env->GetFieldID(cls, "green", "I");
    vc.green = env->GetIntField(jvc, fid);
    
    fid = env->GetFieldID(cls, "blue", "I");
    vc.blue = env->GetIntField(jvc, fid);

    fid = env->GetFieldID(cls, "alpha", "I");
    vc.alpha = env->GetIntField(jvc, fid);

    fid = env->GetFieldID(cls, "depth", "I");
    vc.depth = env->GetIntField(jvc, fid);

    fid = env->GetFieldID(cls, "buffer", "I");
    vc.buffer = env->GetIntField(jvc, fid);
}

/** 
 * Creates a SceneInfo Java object from a Scene.
 * 
 * @param env the JNIEnv
 */
static jobject
scene_info_from_scene(JNIEnv *env, Scene &scene)
{
    jclass cls = env->FindClass("org/linaro/glmark2/SceneInfo");
    jmethodID constructor = env->GetMethodID(cls, "<init>", "(Ljava/lang/String;)V");
    jmethodID add_option = env->GetMethodID(cls, "addOption", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V");

    /* Create the SceneInfo object */
    jstring name = env->NewStringUTF(scene.name().c_str());
    jobject scene_info = env->NewObject(cls, constructor, name);

    const std::map<std::string, Scene::Option> &options = scene.options();

    /* Add options to the SceneInfo object */
    for (std::map<std::string, Scene::Option>::const_iterator opt_iter = options.begin();
         opt_iter != options.end();
         opt_iter++)
    {
        const Scene::Option &opt = opt_iter->second;
        jstring opt_name = env->NewStringUTF(opt.name.c_str());
        jstring opt_description = env->NewStringUTF(opt.description.c_str());
        jstring opt_default_value = env->NewStringUTF(opt.default_value.c_str());

        /* Create and populate the acceptable values array */
        jclass string_cls = env->FindClass("java/lang/String");
        jobjectArray opt_acceptable_values = env->NewObjectArray(opt.acceptable_values.size(),
                                                                 string_cls, 0);
        
        for (size_t i = 0; i < opt.acceptable_values.size(); i++) {
            jstring opt_value = env->NewStringUTF(opt.acceptable_values[i].c_str());
            env->SetObjectArrayElement(opt_acceptable_values, i, opt_value);
            env->DeleteLocalRef(opt_value);
        }

        env->CallVoidMethod(scene_info, add_option,
                            opt_name,
                            opt_description,
                            opt_default_value,
                            opt_acceptable_values);

        env->DeleteLocalRef(opt_name);
        env->DeleteLocalRef(opt_description);
        env->DeleteLocalRef(opt_default_value);
        env->DeleteLocalRef(opt_acceptable_values);
    }

    return scene_info;
}

class DummyCanvas : public Canvas {
public:
    DummyCanvas() : Canvas(0, 0) {}
};

void
Java_org_linaro_glmark2_native_init(JNIEnv* env, jclass clazz,
                                    jobject asset_manager,
                                    jstring args,
                                    jstring log_file)
{
    static_cast<void>(clazz);
    static const std::string arguments_file("/data/glmark2/args");
    int argc = 0;
    char **argv = 0;

    /* Load arguments from argument string or arguments file and parse them */
    if (args) {
        if (env->GetStringUTFLength(args) > 0) {
            const char *args_c_str = env->GetStringUTFChars(args, 0);
            if (args_c_str) {
                get_args_from_string(std::string(args_c_str), argc, argv);
                env->ReleaseStringUTFChars(args, args_c_str);
            }
        }
    }
    else {
        get_args_from_file(arguments_file, argc, argv);
    }

    Options::parse_args(argc, argv);
    release_args(argc, argv);

    /* Get the log file path and open the log file */
    const char *log_file_c_str = env->GetStringUTFChars(log_file, 0);
    if (log_file_c_str) {
        g_log_extra = new std::ofstream(log_file_c_str, std::ios::binary);
        env->ReleaseStringUTFChars(log_file, log_file_c_str);
    }

    /* Force reuse of EGL/GL context */
    Options::reuse_context = true;

    Log::init("glmark2", Options::show_debug, g_log_extra);
    Util::android_set_asset_manager(AAssetManager_fromJava(env, asset_manager));

    g_canvas = new CanvasAndroid(100, 100);
    g_canvas->init();

    Log::info("glmark2 %s\n", GLMARK_VERSION);
    g_canvas->print_info();

    /* Add and register scenes */
    g_scene_collection = new SceneCollection(*g_canvas);
    g_scene_collection->register_scenes();

    g_benchmark_collection = new BenchmarkCollection();
    g_benchmark_collection->populate_from_options();

    if (g_benchmark_collection->needs_decoration()) {
        g_loop = new MainLoopDecorationAndroid(*g_canvas,
                                               g_benchmark_collection->benchmarks());
    }
    else {
        g_loop = new MainLoopAndroid(*g_canvas,
                                     g_benchmark_collection->benchmarks());
    }
}

void
Java_org_linaro_glmark2_native_resize(JNIEnv* env,
                                      jclass clazz,
                                      jint w,
                                      jint h)
{
    static_cast<void>(env);
    static_cast<void>(clazz);

    Log::debug("Resizing to %d x %d\n", w, h);
    g_canvas->resize(w, h);
}

void
Java_org_linaro_glmark2_native_done(JNIEnv* env)
{
    static_cast<void>(env);

    delete g_loop;
    delete g_benchmark_collection;
    delete g_scene_collection;
    delete g_canvas;
    delete g_log_extra;
}

jboolean
Java_org_linaro_glmark2_native_render(JNIEnv* env)
{
    static_cast<void>(env);

    if (!g_loop->step()) {
        Log::info("glmark2 Score: %u\n", g_loop->score());
        return false;
    }

    return true;
}

jint
Java_org_linaro_glmark2_native_scoreConfig(JNIEnv* env, jclass clazz,
                                           jobject jvc, jobject jtarget)
{
    static_cast<void>(clazz);

    GLVisualConfig vc;
    GLVisualConfig target;

    gl_visual_config_from_jobject(env, jvc, vc);
    gl_visual_config_from_jobject(env, jtarget, target);

    return vc.match_score(target);
}

jobjectArray
Java_org_linaro_glmark2_native_getSceneInfo(JNIEnv* env, jclass clazz,
                                            jobject asset_manager)
{
    static_cast<void>(clazz);

    Util::android_set_asset_manager(AAssetManager_fromJava(env, asset_manager));

    DummyCanvas canvas;
    SceneCollection sc(canvas);
    const std::vector<Scene*>& scenes = sc.get();
    std::vector<jobject> si_vector;

    /* Create SceneInfo instances for all the scenes */
    for (std::vector<Scene*>::const_iterator iter = scenes.begin();
         iter != scenes.end();
         iter++)
    {
        jobject si = scene_info_from_scene(env, **iter);
        si_vector.push_back(si);
    }

    /* Create a SceneInfo[] array */
    jclass si_cls = env->FindClass("org/linaro/glmark2/SceneInfo");
    jobjectArray si_array = env->NewObjectArray(si_vector.size(), si_cls, 0);
    
    /* Populate the SceneInfo[] array */
    for (size_t i = 0; i < si_vector.size(); i++)
        env->SetObjectArrayElement(si_array, i, si_vector[i]);

    return si_array;
}

static JNINativeMethod glmark2_native_methods[] = {
    {
        "init",
        "(Landroid/content/res/AssetManager;Ljava/lang/String;Ljava/lang/String;)V",
        reinterpret_cast<void*>(Java_org_linaro_glmark2_native_init)
    },
    {
        "resize",
        "(II)V",
        reinterpret_cast<void*>(Java_org_linaro_glmark2_native_resize)
    },
    {
        "done",
        "()V",
        reinterpret_cast<void*>(Java_org_linaro_glmark2_native_done)
    },
    {
        "render",
        "()Z",
        reinterpret_cast<void*>(Java_org_linaro_glmark2_native_render)
    },
    {
        "scoreConfig",
        "(Lorg/linaro/glmark2/GLVisualConfig;Lorg/linaro/glmark2/GLVisualConfig;)I",
        reinterpret_cast<void*>(Java_org_linaro_glmark2_native_scoreConfig)
    },
    {
        "getSceneInfo",
        "(Landroid/content/res/AssetManager;)[Lorg/linaro/glmark2/SceneInfo;",
        reinterpret_cast<void*>(Java_org_linaro_glmark2_native_getSceneInfo)
    }
};

static int
register_native_methods(JNIEnv* env, const char* className,
                        JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;

    clazz = env->FindClass(className);
    if (clazz == NULL) {
        Log::error("Native registration unable to find class '%s'\n",
                   className);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        Log::error("RegisterNatives failed for '%s'\n", className);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

static int
register_natives(JNIEnv *env)
{
    const char* const class_path_name = "org/linaro/glmark2/Glmark2Native";
    return register_native_methods(env, class_path_name,
                                   glmark2_native_methods,
                                   sizeof(glmark2_native_methods) /
                                   sizeof(glmark2_native_methods[0]));
}

/*
 * Returns the JNI version on success, -1 on failure.
 */
extern "C" jint
JNI_OnLoad(JavaVM* vm, void* reserved)
{
    static_cast<void>(reserved);
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_4) != JNI_OK) {
        Log::error("JNI_OnLoad: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (!register_natives(env)) {
        Log::error("JNI_OnLoad: glmark2 native registration failed\n");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}
