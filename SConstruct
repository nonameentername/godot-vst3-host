#!/usr/bin/env python
import os
import sys


def normalize_path(val, env):
    return val if os.path.isabs(val) else os.path.join(env.Dir("#").abspath, val)


def validate_parent_dir(key, val, env):
    if not os.path.isdir(normalize_path(os.path.dirname(val), env)):
        raise UserError("'%s' is not a directory: %s" % (key, os.path.dirname(val)))


def set_build_info(source, target):
    with open(source, 'r') as f:
        content = f.read()

    content = content.replace("${version}", os.environ.get('TAG_VERSION', '0.0.0'))
    content = content.replace("${build}", os.environ.get('BUILD_SHA', 'none'))

    with open(target, 'w') as f:
        f.write(content) 


build_info_source_files = ["templates/godotvst3host.gdextension.in",
                           "templates/version_generated.gen.h.in"]

build_info_target_files = ["addons/vst3-host/bin/godotvst3host.gdextension",
                           "src/version_generated.gen.h"]


def create_build_info_files(target, source, env):
    for i in range(len(build_info_source_files)):
        set_build_info(build_info_source_files[i], build_info_target_files[i])


def create_build_info_strfunction(target, source, env):
    return f"Creating build info files: {', '.join(str(t) for t in target)}"


libname = "libvst3hostgodot"
projectdir = "."

if sys.platform == "windows":
    localEnv = Environment(tools=["mingw"], PLATFORM="")
else:
    localEnv = Environment(tools=["default"], PLATFORM="")

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Add(
    BoolVariable(
        key="compiledb",
        help="Generate compilation DB (`compile_commands.json`) for external tools",
        default=localEnv.get("compiledb", False),
    )
)
opts.Add(
    PathVariable(
        key="compiledb_file",
        help="Path to a custom `compile_commands.json` file",
        default=localEnv.get("compiledb_file", "compile_commands.json"),
        validator=validate_parent_dir,
    )
)
opts.Add(
    BoolVariable(
        key="asan",
        help="Enable AddressSanitizer for builds",
        default=localEnv.get("asan", False),
    )
)
opts.Update(localEnv)

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()
env["compiledb"] = False

env.Tool("compilation_db")
compilation_db = env.CompilationDatabase(
    normalize_path(localEnv["compiledb_file"], localEnv)
)
env.Alias("compiledb", compilation_db)

env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})

if env["platform"] == "windows":
    env.Append(CPPFLAGS=["-DMINGW"])
    env.Append(LIBS=["sdk_hosting", "sdk", "sdk_common", "base", "pluginterfaces"])
    env.Append(LIBS=["uuid", "ole32"])

    if env["dev_build"]:
        env.Append(LIBPATH=["addons/vst3-host/bin/windows/debug/vcpkg_installed/x64-mingw-static/debug/lib",
                            "addons/vst3-host/bin/windows/debug/vcpkg_installed/x64-mingw-static/debug/lib/vst3sdk/Debug"])
        env.Append(CPPPATH=["addons/vst3-host/bin/windows/debug/vcpkg_installed/x64-mingw-static/include",
                            "addons/vst3-host/bin/windows/debug/vcpkg_installed/x64-mingw-static/include/vst3sdk"])
    else:
        env.Append(LIBPATH=["addons/vst3-host/bin/windows/release/vcpkg_installed/x64-mingw-static/lib",
                            "addons/vst3-host/bin/windows/release/vcpkg_installed/x64-mingw-static/lib/vst3sdk/Release"])
        env.Append(CPPPATH=["addons/vst3-host/bin/windows/release/vcpkg_installed/x64-mingw-static/include",
                            "addons/vst3-host/bin/windows/release/vcpkg_installed/x64-mingw-static/include/vst3sdk"])
elif env["platform"] == "web":
    if env["dev_build"]:
        env.Append(CPPFLAGS=["-g"])
        env.Append(LINKFLAGS=["-g", "-s", "ERROR_ON_UNDEFINED_SYMBOLS=1"])
elif env["platform"] == "macos":
    env.Append(LINKFLAGS=["-framework", "CoreAudio",
                          "-framework", "AudioToolbox",
                          "-framework", "CoreFoundation",
                          "-framework", "Foundation"
                          ])
    env.Append(LIBS=["sdk_hosting", "sdk", "sdk_common", "base", "pluginterfaces"])

    if env["dev_build"]:
        env.Append(LIBPATH=["addons/vst3-host/bin/osxcross/debug/vcpkg_installed/universal-osxcross/debug/lib",
                            "addons/vst3-host/bin/osxcross/debug/vcpkg_installed/universal-osxcross/debug/lib/vst3sdk/Debug"])
        env.Append(CPPPATH=["addons/vst3-host/bin/osxcross/debug/vcpkg_installed/universal-osxcross/include",
                            "addons/vst3-host/bin/osxcross/debug/vcpkg_installed/universal-osxcross/include/vst3sdk"])
    else:
        env.Append(LIBPATH=["addons/vst3-host/bin/osxcross/release/vcpkg_installed/universal-osxcross/lib",
                            "addons/vst3-host/bin/osxcross/release/vcpkg_installed/universal-osxcross/lib/vst3sdk/Release"])
        env.Append(CPPPATH=["addons/vst3-host/bin/osxcross/release/vcpkg_installed/universal-osxcross/include",
                            "addons/vst3-host/bin/osxcross/release/vcpkg_installed/universal-osxcross/include/vst3sdk"])
elif env["platform"] == "linux":
    env.Append(LIBS=["sdk_hosting", "sdk", "sdk_common", "base", "pluginterfaces"])

    if env["dev_build"]:
        env.Append(LIBPATH=["addons/vst3-host/bin/linux/debug/vcpkg_installed/x64-linux/debug/lib",
                            "addons/vst3-host/bin/linux/debug/vcpkg_installed/x64-linux/debug/lib/vst3sdk/Debug"])
        env.Append(CPPPATH=["addons/vst3-host/bin/linux/debug/vcpkg_installed/x64-linux/include",
                            "addons/vst3-host/bin/linux/debug/vcpkg_installed/x64-linux/include/vst3sdk"])
        #env.Append(RPATH=["", "."])
    else:
        env.Append(LIBPATH=["addons/vst3-host/bin/linux/release/vcpkg_installed/x64-linux/lib",
                            "addons/vst3-host/bin/linux/release/vcpkg_installed/x64-linux/lib/vst3sdk/Release"])
        env.Append(CPPPATH=["addons/vst3-host/bin/linux/release/vcpkg_installed/x64-linux/include",
                            "addons/vst3-host/bin/linux/release/vcpkg_installed/x64-linux/include/vst3sdk"])
        #env.Append(RPATH=["", "."])

#env.Append(CPPFLAGS=["-fexceptions"])

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")
sources.append("src/thirdparty/vst3/hosting/plugprovider.cpp")

#TODO: handle other operating systems
if env["platform"] == "windows":
    sources.append("src/thirdparty/vst3/hosting/module_win32.cpp")
elif env["platform"] == "linux":
    sources.append("src/thirdparty/vst3/hosting/module_linux.cpp")
elif env["platform"] == "macos":
    sources.append("src/thirdparty/vst3/hosting/module_mac.mm")
    sources.append("src/thirdparty/vst3/hosting/threadchecker_mac.mm")

if env.get("asan", False):
    print("SCons: Building with AddressSanitizer instrumentation")
    env.Append(CPPFLAGS=["-fsanitize=address", "-fno-omit-frame-pointer", "-g"])
    env.Append(LINKFLAGS=["-fsanitize=address"])

if env["target"] in ["editor", "template_debug"]:
	try:
		doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
		sources.append(doc_data)
	except AttributeError:
		print("Not including class reference as we're targeting a pre-4.3 baseline.")

file = "{}{}{}".format(libname, env["suffix"], env["SHLIBSUFFIX"])

if env["platform"] == "macos":
    platlibname = "{}.{}.{}".format(libname, env["platform"], env["target"])
    file = "{}.framework/{}".format(env["platform"], platlibname, platlibname)

libraryfile = "addons/vst3-host/bin/{}/{}".format(env["platform"], file)
library = env.SharedLibrary(
    libraryfile,
    source=sources,
)

create_build_info_action = Action(create_build_info_files,
                                  strfunction=create_build_info_strfunction)

env.Command(
    build_info_target_files,
    build_info_source_files,
    create_build_info_action
)

env.AddPreAction(library, create_build_info_action)
env.Depends(library, build_info_target_files)

#copy = env.InstallAs("{}/bin/{}/lib{}".format(projectdir, env["platform"], file), library)

default_args = [library]
if localEnv.get("compiledb", False):
    default_args += [compilation_db]
Default(*default_args)

