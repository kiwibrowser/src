/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Android utilities.
 *//*--------------------------------------------------------------------*/

#include "tcuAndroidUtil.hpp"

#include "deSTLUtil.hpp"
#include "deMath.h"

#include <vector>

namespace tcu
{
namespace Android
{

using std::string;
using std::vector;

namespace
{

class ScopedJNIEnv
{
public:

					ScopedJNIEnv	(JavaVM* vm);
					~ScopedJNIEnv	(void);

	JavaVM*			getVM			(void) const { return m_vm;		}
	JNIEnv*			getEnv			(void) const { return m_env;	}

private:
	JavaVM* const	m_vm;
	JNIEnv*			m_env;
	bool			m_detach;
};

ScopedJNIEnv::ScopedJNIEnv (JavaVM* vm)
	: m_vm		(vm)
	, m_env		(DE_NULL)
	, m_detach	(false)
{
	const int	getEnvRes	= m_vm->GetEnv((void**)&m_env, JNI_VERSION_1_6);

	if (getEnvRes == JNI_EDETACHED)
	{
		if (m_vm->AttachCurrentThread(&m_env, DE_NULL) != JNI_OK)
			throw std::runtime_error("JNI AttachCurrentThread() failed");

		m_detach = true;
	}
	else if (getEnvRes != JNI_OK)
		throw std::runtime_error("JNI GetEnv() failed");

	DE_ASSERT(m_env);
}

ScopedJNIEnv::~ScopedJNIEnv (void)
{
	if (m_detach)
		m_vm->DetachCurrentThread();
}

class LocalRef
{
public:
					LocalRef		(JNIEnv* env, jobject ref);
					~LocalRef		(void);

	jobject			operator*		(void) const { return m_ref;	}
	operator		bool			(void) const { return !!m_ref;	}

private:
					LocalRef		(const LocalRef&);
	LocalRef&		operator=		(const LocalRef&);

	JNIEnv* const	m_env;
	const jobject	m_ref;
};

LocalRef::LocalRef (JNIEnv* env, jobject ref)
	: m_env(env)
	, m_ref(ref)
{
}

LocalRef::~LocalRef (void)
{
	if (m_ref)
		m_env->DeleteLocalRef(m_ref);
}

void checkException (JNIEnv* env)
{
	if (env->ExceptionCheck())
	{
		env->ExceptionDescribe();
		env->ExceptionClear();
		throw std::runtime_error("Got JNI exception");
	}
}

jclass findClass (JNIEnv* env, const char* className)
{
	const jclass	cls		= env->FindClass(className);

	checkException(env);
	TCU_CHECK_INTERNAL(cls);

	return cls;
}

jclass getObjectClass (JNIEnv* env, jobject object)
{
	const jclass	cls		= env->GetObjectClass(object);

	checkException(env);
	TCU_CHECK_INTERNAL(cls);

	return cls;
}

jmethodID getMethodID (JNIEnv* env, jclass cls, const char* methodName, const char* signature)
{
	const jmethodID		id		= env->GetMethodID(cls, methodName, signature);

	checkException(env);
	TCU_CHECK_INTERNAL(id);

	return id;
}

string getStringValue (JNIEnv* env, jstring jniStr)
{
	const char*		ptr		= env->GetStringUTFChars(jniStr, DE_NULL);
	const string	str		= string(ptr);

	env->ReleaseStringUTFChars(jniStr, ptr);

	return str;
}

string getIntentStringExtra (JNIEnv* env, jobject activity, const char* name)
{
	// \todo [2013-05-12 pyry] Clean up references on error.

	const jclass	activityCls		= getObjectClass(env, activity);
	const LocalRef	intent			(env, env->CallObjectMethod(activity, getMethodID(env, activityCls, "getIntent", "()Landroid/content/Intent;")));
	TCU_CHECK_INTERNAL(intent);

	const LocalRef	extraName		(env, env->NewStringUTF(name));
	const jclass	intentCls		= getObjectClass(env, *intent);
	TCU_CHECK_INTERNAL(extraName && intentCls);

	jvalue getExtraArgs[1];
	getExtraArgs[0].l = *extraName;

	const LocalRef	extraStr		(env, env->CallObjectMethodA(*intent, getMethodID(env, intentCls, "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;"), getExtraArgs));

	if (extraStr)
		return getStringValue(env, (jstring)*extraStr);
	else
		return string();
}

void setRequestedOrientation (JNIEnv* env, jobject activity, ScreenOrientation orientation)
{
	const jclass	activityCls			= getObjectClass(env, activity);
	const jmethodID	setOrientationId	= getMethodID(env, activityCls, "setRequestedOrientation", "(I)V");

	env->CallVoidMethod(activity, setOrientationId, (int)orientation);
}

template<typename Type>
const char* getJNITypeStr (void);

template<>
const char* getJNITypeStr<int> (void)
{
	return "I";
}

template<>
const char* getJNITypeStr<float> (void)
{
	return "F";
}

template<>
const char* getJNITypeStr<string> (void)
{
	return "Ljava/lang/String;";
}

template<>
const char* getJNITypeStr<vector<string> > (void)
{
	return "[Ljava/lang/String;";
}

template<typename FieldType>
FieldType getStaticFieldValue (JNIEnv* env, jclass cls, jfieldID fieldId);

template<>
int getStaticFieldValue<int> (JNIEnv* env, jclass cls, jfieldID fieldId)
{
	DE_ASSERT(cls && fieldId);
	return env->GetStaticIntField(cls, fieldId);
}

template<>
string getStaticFieldValue<string> (JNIEnv* env, jclass cls, jfieldID fieldId)
{
	const jstring	jniStr	= (jstring)env->GetStaticObjectField(cls, fieldId);

	if (jniStr)
		return getStringValue(env, jniStr);
	else
		return string();
}

template<>
vector<string> getStaticFieldValue<vector<string> > (JNIEnv* env, jclass cls, jfieldID fieldId)
{
	const jobjectArray	array		= (jobjectArray)env->GetStaticObjectField(cls, fieldId);
	vector<string>		result;

	checkException(env);

	if (array)
	{
		const int	numElements		= env->GetArrayLength(array);

		for (int ndx = 0; ndx < numElements; ndx++)
		{
			const jstring	jniStr	= (jstring)env->GetObjectArrayElement(array, ndx);

			checkException(env);

			if (jniStr)
				result.push_back(getStringValue(env, jniStr));
		}
	}

	return result;
}

template<typename FieldType>
FieldType getStaticField (JNIEnv* env, const char* className, const char* fieldName)
{
	const jclass	cls			= findClass(env, className);
	const jfieldID	fieldId		= env->GetStaticFieldID(cls, fieldName, getJNITypeStr<FieldType>());

	checkException(env);

	if (fieldId)
		return getStaticFieldValue<FieldType>(env, cls, fieldId);
	else
		throw std::runtime_error(string(fieldName) + " not found in " + className);
}

template<typename FieldType>
FieldType getFieldValue (JNIEnv* env, jobject obj, jfieldID fieldId);

template<>
int getFieldValue<int> (JNIEnv* env, jobject obj, jfieldID fieldId)
{
	DE_ASSERT(obj && fieldId);
	return env->GetIntField(obj, fieldId);
}

template<>
float getFieldValue<float> (JNIEnv* env, jobject obj, jfieldID fieldId)
{
	DE_ASSERT(obj && fieldId);
	return env->GetFloatField(obj, fieldId);
}

template<typename FieldType>
FieldType getField (JNIEnv* env, jobject obj, const char* fieldName)
{
	const jclass	cls			= getObjectClass(env, obj);
	const jfieldID	fieldId		= env->GetFieldID(cls, fieldName, getJNITypeStr<FieldType>());

	checkException(env);

	if (fieldId)
		return getFieldValue<FieldType>(env, obj, fieldId);
	else
		throw std::runtime_error(string(fieldName) + " not found in object");
}

void describePlatform (JNIEnv* env, std::ostream& dst)
{
	const char* const	buildClass		= "android/os/Build";
	const char* const	versionClass	= "android/os/Build$VERSION";

	static const struct
	{
		const char*		classPath;
		const char*		className;
		const char*		fieldName;
	} s_stringFields[] =
	{
		{ buildClass,	"Build",			"BOARD"			},
		{ buildClass,	"Build",			"BRAND"			},
		{ buildClass,	"Build",			"DEVICE"		},
		{ buildClass,	"Build",			"DISPLAY"		},
		{ buildClass,	"Build",			"FINGERPRINT"	},
		{ buildClass,	"Build",			"HARDWARE"		},
		{ buildClass,	"Build",			"MANUFACTURER"	},
		{ buildClass,	"Build",			"MODEL"			},
		{ buildClass,	"Build",			"PRODUCT"		},
		{ buildClass,	"Build",			"TAGS"			},
		{ buildClass,	"Build",			"TYPE"			},
		{ versionClass,	"Build.VERSION",	"RELEASE"		},
	};

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_stringFields); ndx++)
		dst << s_stringFields[ndx].className << "." << s_stringFields[ndx].fieldName
			<< ": " << getStaticField<string>(env, s_stringFields[ndx].classPath, s_stringFields[ndx].fieldName)
			<< "\n";

	dst << "Build.VERSION.SDK_INT: " << getStaticField<int>(env, versionClass, "SDK_INT") << "\n";

	{
		const vector<string>	supportedAbis	= getStaticField<vector<string> >(env, buildClass, "SUPPORTED_ABIS");

		dst << "Build.SUPPORTED_ABIS: ";

		for (size_t ndx = 0; ndx < supportedAbis.size(); ndx++)
			dst << (ndx != 0 ? ", " : "") << supportedAbis[ndx];

		dst << "\n";
	}
}

vector<string> getSupportedABIs (JNIEnv* env)
{
	return getStaticField<vector<string> >(env, "android/os/Build", "SUPPORTED_ABIS");
}

bool supportsAny64BitABI (JNIEnv* env)
{
	const vector<string>	supportedAbis		= getSupportedABIs(env);
	const char*				known64BitAbis[]	= { "arm64-v8a", "x86_64", "mips64" };

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(known64BitAbis); ++ndx)
	{
		if (de::contains(supportedAbis.begin(), supportedAbis.end(), string(known64BitAbis[ndx])))
			return true;
	}

	return false;
}

bool supportsAny64BitABI (ANativeActivity* activity)
{
	const ScopedJNIEnv	env(activity->vm);

	return supportsAny64BitABI(env.getEnv());
}

jobject getPackageManager (JNIEnv* env, jobject activity)
{
	const jclass		activityCls		= getObjectClass(env, activity);
	const jmethodID		getPMID			= getMethodID(env, activityCls, "getPackageManager", "()Landroid/content/pm/PackageManager;");
	const jobject		packageManager	= env->CallObjectMethod(activity, getPMID);

	return packageManager;
}

bool hasSystemFeature (JNIEnv* env, jobject activity, const char* name)
{
	const LocalRef		packageManager	(env, getPackageManager(env, activity));
	const jclass		pmCls			= getObjectClass(env, *packageManager);
	const jmethodID		hasFeatureID	= getMethodID(env, pmCls, "hasSystemFeature", "(Ljava/lang/String;)Z");
	const LocalRef		nameStr			(env, env->NewStringUTF(name));
	jvalue				callArgs[1];

	callArgs[0].l = *nameStr;

	return env->CallBooleanMethodA(*packageManager, hasFeatureID, callArgs) == JNI_TRUE;
}

jobject getWindowManager (JNIEnv* env, jobject activity)
{
	const jclass		activityCls		= getObjectClass(env, activity);
	const jmethodID		getWMID			= getMethodID(env, activityCls, "getWindowManager", "()Landroid/view/WindowManager;");
	const jobject		windowManager	= env->CallObjectMethod(activity, getWMID);

	return windowManager;
}

jobject getDefaultDisplay (JNIEnv* env, jobject windowManager)
{
	const jclass		wmClass			= getObjectClass(env, windowManager);
	const jmethodID		getDisplayID	= getMethodID(env, wmClass, "getDefaultDisplay", "()Landroid/view/Display;");
	const jobject		display			= env->CallObjectMethod(windowManager, getDisplayID);

	return display;
}

jobject createDisplayMetrics (JNIEnv* env)
{
	const jclass		displayMetricsCls	= findClass(env, "android/util/DisplayMetrics");
	const jmethodID		ctorId				= getMethodID(env, displayMetricsCls, "<init>", "()V");

	return env->NewObject(displayMetricsCls, ctorId);
}

DisplayMetrics getDisplayMetrics (JNIEnv* env, jobject activity)
{
	const LocalRef		windowManager		(env, getWindowManager(env, activity));
	const LocalRef		defaultDisplay		(env, getDefaultDisplay(env, *windowManager));
	const LocalRef		nativeMetrics		(env, createDisplayMetrics(env));
	const jclass		displayCls			= getObjectClass(env, *defaultDisplay);
	const jmethodID		getMetricsID		= getMethodID(env, displayCls, "getMetrics", "(Landroid/util/DisplayMetrics;)V");
	DisplayMetrics		metrics;

	{
		jvalue callArgs[1];
		callArgs[0].l = *nativeMetrics;

		env->CallVoidMethodA(*defaultDisplay, getMetricsID, callArgs);
	}

	metrics.density			= getField<float>	(env, *nativeMetrics, "density");
	metrics.densityDpi		= getField<int>		(env, *nativeMetrics, "densityDpi");
	metrics.scaledDensity	= getField<float>	(env, *nativeMetrics, "scaledDensity");
	metrics.widthPixels		= getField<int>		(env, *nativeMetrics, "widthPixels");
	metrics.heightPixels	= getField<int>		(env, *nativeMetrics, "heightPixels");
	metrics.xdpi			= getField<float>	(env, *nativeMetrics, "xdpi");
	metrics.ydpi			= getField<float>	(env, *nativeMetrics, "ydpi");

	return metrics;
}

enum ScreenClass
{
	SCREEN_CLASS_WEAR	= 0,
	SCREEN_CLASS_SMALL,
	SCREEN_CLASS_NORMAL,
	SCREEN_CLASS_LARGE,
	SCREEN_CLASS_EXTRA_LARGE,

	SCREEN_CLASS_LAST
};

enum DensityClass
{
	DENSITY_CLASS_LDPI		= 120,
	DENSITY_CLASS_MDPI		= 160,
	DENSITY_CLASS_TVDPI		= 213,
	DENSITY_CLASS_HDPI		= 240,
	DENSITY_CLASS_280DPI	= 280,
	DENSITY_CLASS_XHDPI		= 320,
	DENSITY_CLASS_360DPI	= 360,
	DENSITY_CLASS_400DPI	= 400,
	DENSITY_CLASS_420DPI	= 420,
	DENSITY_CLASS_XXHDPI	= 480,
	DENSITY_CLASS_560DPI	= 560,
	DENSITY_CLASS_XXXHDPI	= 640,

	DENSITY_CLASS_INVALID	= -1,
};

ScreenClass getScreenClass (const DisplayMetrics& displayMetrics)
{
	static const struct
	{
		int			minWidthDp;
		int			minHeightDp;
		ScreenClass	screenClass;
	} s_screenClasses[] =
	{
		// Must be ordered from largest to smallest
		{ 960, 720,		SCREEN_CLASS_EXTRA_LARGE	},
		{ 640, 480,		SCREEN_CLASS_LARGE			},
		{ 480, 320,		SCREEN_CLASS_NORMAL			},
		{ 426, 320,		SCREEN_CLASS_SMALL			},
	};

	const float		dpScale		= float(displayMetrics.densityDpi) / 160.f;

	// \note Assume landscape orientation for comparison
	const int		widthP		= de::max(displayMetrics.widthPixels, displayMetrics.heightPixels);
	const int		heightP		= de::min(displayMetrics.widthPixels, displayMetrics.heightPixels);

	const int		widthDp		= deFloorFloatToInt32(float(widthP) / dpScale);
	const int		heightDp	= deFloorFloatToInt32(float(heightP) / dpScale);

	for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_screenClasses); ++ndx)
	{
		if ((s_screenClasses[ndx].minWidthDp <= widthDp) &&
			(s_screenClasses[ndx].minHeightDp <= heightDp))
			return s_screenClasses[ndx].screenClass;
	}

	return SCREEN_CLASS_WEAR;
}

bool isValidDensityClass (int dpi)
{
	switch (dpi)
	{
		case DENSITY_CLASS_LDPI:
		case DENSITY_CLASS_MDPI:
		case DENSITY_CLASS_TVDPI:
		case DENSITY_CLASS_HDPI:
		case DENSITY_CLASS_280DPI:
		case DENSITY_CLASS_XHDPI:
		case DENSITY_CLASS_360DPI:
		case DENSITY_CLASS_400DPI:
		case DENSITY_CLASS_420DPI:
		case DENSITY_CLASS_XXHDPI:
		case DENSITY_CLASS_560DPI:
		case DENSITY_CLASS_XXXHDPI:
			return true;

		default:
			return false;
	}
}

DensityClass getDensityClass (const DisplayMetrics& displayMetrics)
{
	if (isValidDensityClass(displayMetrics.densityDpi))
		return (DensityClass)displayMetrics.densityDpi;
	else
		return DENSITY_CLASS_INVALID;
}

} // anonymous

ScreenOrientation mapScreenRotation (ScreenRotation rotation)
{
	switch (rotation)
	{
		case SCREENROTATION_UNSPECIFIED:	return SCREEN_ORIENTATION_UNSPECIFIED;
		case SCREENROTATION_0:				return SCREEN_ORIENTATION_PORTRAIT;
		case SCREENROTATION_90:				return SCREEN_ORIENTATION_LANDSCAPE;
		case SCREENROTATION_180:			return SCREEN_ORIENTATION_REVERSE_PORTRAIT;
		case SCREENROTATION_270:			return SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
		default:
			print("Warning: Unsupported rotation");
			return SCREEN_ORIENTATION_PORTRAIT;
	}
}

string getIntentStringExtra (ANativeActivity* activity, const char* name)
{
	const ScopedJNIEnv	env(activity->vm);

	return getIntentStringExtra(env.getEnv(), activity->clazz, name);
}

void setRequestedOrientation (ANativeActivity* activity, ScreenOrientation orientation)
{
	const ScopedJNIEnv	env(activity->vm);

	setRequestedOrientation(env.getEnv(), activity->clazz, orientation);
}

void describePlatform (ANativeActivity* activity, std::ostream& dst)
{
	const ScopedJNIEnv	env(activity->vm);

	describePlatform(env.getEnv(), dst);
}

bool hasSystemFeature (ANativeActivity* activity, const char* name)
{
	const ScopedJNIEnv	env(activity->vm);

	return hasSystemFeature(env.getEnv(), activity->clazz, name);
}

DisplayMetrics getDisplayMetrics (ANativeActivity* activity)
{
	const ScopedJNIEnv	env(activity->vm);

	return getDisplayMetrics(env.getEnv(), activity->clazz);
}

size_t getCDDRequiredSystemMemory (ANativeActivity* activity)
{
	const DisplayMetrics	displayMetrics	= getDisplayMetrics(activity);
	const ScreenClass		screenClass		= getScreenClass(displayMetrics);
	const bool				isWearDevice	= hasSystemFeature(activity, "android.hardware.type.watch");
	const bool				is64BitDevice	= supportsAny64BitABI(activity);
	const size_t			MiB				= (size_t)(1<<20);

	if (!is64BitDevice)
		TCU_CHECK_INTERNAL(sizeof(void*) != sizeof(deUint64));

	if (isWearDevice)
	{
		TCU_CHECK_INTERNAL(!is64BitDevice);
		return 416*MiB;
	}
	else
	{
		const DensityClass	densityClass	= getDensityClass(displayMetrics);

		TCU_CHECK_INTERNAL(de::inRange(screenClass, SCREEN_CLASS_SMALL, SCREEN_CLASS_EXTRA_LARGE));
		TCU_CHECK_INTERNAL(densityClass != DENSITY_CLASS_INVALID);

		static const struct
		{
			DensityClass	smallNormalScreenDensity;
			DensityClass	largeScreenDensity;
			DensityClass	extraLargeScreenDensity;
			size_t			requiredMem32bit;
			size_t			requiredMem64bit;
		} s_classes[] =
		{
			// Must be ordered from largest to smallest
			{ DENSITY_CLASS_560DPI,		DENSITY_CLASS_400DPI,	DENSITY_CLASS_XHDPI,	1344*MiB,	1824*MiB	},
			{ DENSITY_CLASS_400DPI,		DENSITY_CLASS_XHDPI,	DENSITY_CLASS_TVDPI,	896*MiB,	1280*MiB	},
			{ DENSITY_CLASS_XHDPI,		DENSITY_CLASS_HDPI,		DENSITY_CLASS_MDPI,		512*MiB,	832*MiB		},

			// \note Last is default, and density values are maximum allowed
			{ DENSITY_CLASS_280DPI,		DENSITY_CLASS_MDPI,		DENSITY_CLASS_LDPI,		424*MiB,	704*MiB		},
		};

		for (int ndx = 0; ndx < DE_LENGTH_OF_ARRAY(s_classes); ++ndx)
		{
			const DensityClass	minClass	= screenClass == SCREEN_CLASS_EXTRA_LARGE	? s_classes[ndx].extraLargeScreenDensity
											: screenClass == SCREEN_CLASS_LARGE			? s_classes[ndx].largeScreenDensity
											: /* small/normal */						  s_classes[ndx].smallNormalScreenDensity;
			const size_t		reqMem		= is64BitDevice ? s_classes[ndx].requiredMem64bit : s_classes[ndx].requiredMem32bit;
			const bool			isLast		= ndx == DE_LENGTH_OF_ARRAY(s_classes)-1;

			if ((isLast && minClass >= densityClass) || (!isLast && minClass <= densityClass))
				return reqMem;
		}

		TCU_THROW(InternalError, "Invalid combination of density and screen size");
	}
}

} // Android
} // tcu
