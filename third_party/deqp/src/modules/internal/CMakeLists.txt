# drawElements internal tests

set(DE_INTERNAL_TESTS_SRCS
	ditBuildInfoTests.cpp
	ditBuildInfoTests.hpp
	ditDelibsTests.cpp
	ditDelibsTests.hpp
	ditFrameworkTests.cpp
	ditFrameworkTests.hpp
	ditImageCompareTests.cpp
	ditImageCompareTests.hpp
	ditImageIOTests.cpp
	ditImageIOTests.hpp
	ditTestCase.cpp
	ditTestCase.hpp
	ditTestLogTests.cpp
	ditTestLogTests.hpp
	ditTestPackage.cpp
	ditTestPackage.hpp
	ditSeedBuilderTests.hpp
	ditSeedBuilderTests.cpp
	ditSRGB8ConversionTest.hpp
	ditSRGB8ConversionTest.cpp
	ditTextureFormatTests.cpp
	ditTextureFormatTests.hpp
	ditAstcTests.cpp
	ditAstcTests.hpp
	ditVulkanTests.cpp
	ditVulkanTests.hpp
	)

set(DE_INTERNAL_TESTS_LIBS
	tcutil
	referencerenderer
	vkutil
	)

add_deqp_module(de-internal-tests "${DE_INTERNAL_TESTS_SRCS}" "${DE_INTERNAL_TESTS_LIBS}" ditTestPackageEntry.cpp)

add_data_dir(de-internal-tests ../../data/internal/data	internal/data)
