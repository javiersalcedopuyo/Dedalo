// C stuff
#include <cstddef>
#include <cstdlib>
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <strings.h>

#if !defined( __APPLE__ )
    #define ELAST 256
#endif // !__APPLE__

// STL bloat
#include <string>
#include <string_view>
#include <format>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <ranges>

// Multi-threading
#include <thread>
#include <mutex>
#include <stop_token>
using MutexLock = std::scoped_lock<std::mutex>;
using Thread    = std::thread;
using StopSrc   = std::stop_source;
using StopToken = std::stop_token;

// Rust at home ///////////////////////////////////////////////////////////////
#define fun auto
#define var auto
#define let auto const
#define constant static constexpr auto

#define file_private static // For static free functions in cpp files

namespace FS = std::filesystem;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;

using uchar = unsigned char;
using usize = size_t;

using f32 = float;
using f64 = double;

using String = std::string;

template<typename T>
using List = std::vector<T>;

using Path = FS::path;

#define as static_cast


template< typename... Args >
static inline fun strfmt( std::format_string<Args...> fmt_str, Args&&... args ) -> String
{
    return std::format( fmt_str, std::forward<Args>( args )... );
}


template< typename... Args >
static inline fun println( FILE* stream, std::format_string<Args...> fmt_str, Args&&... args )
{
    fprintf( stream, "%s\n", strfmt( fmt_str, std::forward<Args>( args )... ).c_str() );
}


 // TODO: Support MSVC macros
#if defined( ENABLE_LOGS )
    #if defined( NDEBUG )
        #define INFO(...)                println( stderr, "[DEDALO] {}",    strfmt(__VA_ARGS__) )
        #define ERROR(...)               println( stderr, "[DEDALO] ⛔ ERROR: {}",   strfmt(__VA_ARGS__) )
        #define WARNING(...)             ERROR( __VA_ARGS__ ) // On release, treat all warnings as errors
        #define UNIMPLEMENTED_MSG( ... )
    #else // NDEBUG
        #define INFO(...)                println( stderr, "💬 INFO [{} @ {} ln{}]: {}",    __func__, __FILE__, __LINE__, strfmt(__VA_ARGS__) )
        #define WARNING(...)             println( stderr, "⚠️ WARNING [{} @ {} ln{}]: {}", __func__, __FILE__, __LINE__, strfmt(__VA_ARGS__) )
        #define ERROR(...)               println( stderr, "⛔️ ERROR [{} @ {} ln{}]: {}",   __func__, __FILE__, __LINE__, strfmt(__VA_ARGS__) )
        #define UNIMPLEMENTED_MSG( ... ) println( stderr, "🚧 UNIMPLEMENTED [{} @ {} ln{}]: {}", __func__, __FILE__, __LINE__, strfmt(__VA_ARGS__) )
    #endif // NDEBUG
    #define UNIMPLEMENTED()          UNIMPLEMENTED_MSG( "" )
#else // ENABLE_LOGS
    #define INFO( ... )
    #define WARNING( ... )
    #define ERROR( ... )
    #define UNIMPLEMENTED_MSG( ... )
    #define UNIMPLEMENTED()
#endif // ENABLE_LOGS

#define PANIC(...) do{ ERROR( __VA_ARGS__ ); exit( 1 ); } while( false )

#if __cplusplus >= 202302L
    #define ASSUME( exp ) [[ assume( exp ) ]]
#elif defined( __clang__ )
    #define ASSUME( exp ) __builtin_assume( exp )
#elif defined( __GNUC__ )
    #define ASSUME( exp ) if( (exp) == false ) __builtin_unreachable()
#elif defined( _MSC_VER )
    #define ASSUME( exp ) __assume( exp )
#else
    #define ASSUME( exp )
#endif


#if defined( NDEBUG )
//{
    #define REQUIRE( exp )          ASSUME( exp )
    #define REQUIRE_MSG( exp, msg ) ASSUME( exp )
    #define UNREACHABLE             ASSUME( false )
//}
#else
//{
    #define REQUIRE( exp )          if( !(exp) ) PANIC( "Requirement '{}' was not met", #exp )
    #define REQUIRE_MSG( exp, msg ) if( !(exp) ) PANIC( "Requirement '{}' was not met: {}", #exp, msg )
    #define UNREACHABLE             PANIC( "Reached unreachable code." )
//}
#endif


file_private constexpr fun to_lower( String* str )
{
    REQUIRE( str );
    std::ranges::transform( *str, str->begin(), []( uchar c ) -> uchar { return std::tolower( c ); } );
}


struct Platform
{
    enum List: u8
    {
        Linux   = 1 << 0,
        Apple   = 1 << 1,
        Windows = 1 << 2,
        UNIX    = Linux | Apple
    } val;

    [[nodiscard]]
    constexpr fun is( const List p ) const -> bool
    {
        return (p & val) != 0;
    }

    [[nodiscard]]
    constexpr fun static_lib_extension() const -> String
    {
        if( this->is( UNIX ) )
            return ".a";
        else if( this->is( Windows ) )
            return ".lib";
        else
            UNREACHABLE;
    }

    [[nodiscard]]
    constexpr fun dynamic_lib_extension() const -> String
    {
        if( this->is( Linux ) )
            return ".so";
        if( this->is( Apple ) )
            return ".dylib";
        else if( this->is( Windows ) )
            return ".dll";
        else
            UNREACHABLE;
    }

    [[nodiscard]]
    constexpr fun get_name() const -> String
    {
        if( this->is( Linux ) )
            return "Linux";
        if( this->is( Apple ) )
            return "Apple";
        else if( this->is( Windows ) )
            return "Windows";
        else
            UNREACHABLE;
    }
};


static constexpr Platform platform =
#if defined( __linux__ )
    { .val = Platform::Linux };
#elif defined( __APPLE__ )
    { .val = Platform::Apple };
#elif defined( _WIN32 )
    { .val = Platform::Windows };
#else
    #error Unsupported platform
#endif


enum struct Compiler: u8 { Clang, GCC, MSVC };


// TODO: Add more https://clang.llvm.org/docs/UsersManual.html#controlling-code-generation
enum Sanitizer: u8
{
    No_Sanitizers = 0,
    ASan  = 1 << 0,
    UBSan = 1 << 1,
    TSan  = 1 << 2,
};


struct Version
{
    u8 major = 0;
    u8 minor = 0;
    u8 patch = 1;
};


file_private constexpr let version = Version{ 0,3,2 };


struct ScriptPtr
{
    bool (*func)() = nullptr;
    String name = "";
};


struct Target
{

    String       name               = "UNNAMED"; // Redundant but convenient
    u8           optimization_level = 0;
    u8           sanitizers         = No_Sanitizers;
    List<String> defines            = {};
    List<String> compiler_flags     = {};
    List<String> linker_flags       = {};
    List<Path>   ignored_paths      = {};

    List<ScriptPtr> pre_build_scripts  = {};
    List<ScriptPtr> post_build_scripts = {};

    String source_path = "src"; // Only really meant for the test target to have a different one.
};

enum struct Location: u8
{
    System,
    Local,
    Remote,
};

enum struct Linking: u8
{
    Static,
    Dynamic,
    SingleHeader
};

enum struct LTO: u8
{
    None,
    Full,
    Incremental // Only in Clang
};

struct Dependency
{
    String       name         = "UNNAMED";
    Linking      linking      = Linking::Dynamic;
    Location     location     = Location::System;
    String       linker_flags = "";
    Version      version      = { 0,0,1 };
    List<String> targets      = { "All" };
};

#if defined( __APPLE__ )
struct Framework
{
    String       name         = "UNNAMED";
    List<String> targets      = { "All" };
};
#endif // __APPLE__


struct Project
{
    enum Type: u8 { Executable, Library };
    struct Args
    {
        String           name                      = "UNNAMED";
        String           description               = "";
        Version          version                   = {0,0,1};
        List<String>     authors                   = {};
        Type             type                      = Executable;
        Compiler         compiler                  = Compiler::Clang;
        u8               cpp_version               = 20;
        bool             generate_compile_commands = true;
        bool             enable_cpp_extensions     = false;
        List<String>     common_compiler_flags     = {};
        List<String>     common_defines            = {};
        List<String>     command_line_arguments    = {};
        String           default_target            = "debug";
        LTO              link_time_optimizations   = LTO::None;
        List<Dependency> dependencies              = {};
        List<ScriptPtr>  pre_build_scripts         = {};
        List<ScriptPtr>  post_build_scripts        = {};
    #if defined( __APPLE__ )
        List<Framework>  frameworks                = {};
    #endif
    };

    Project() = default;
    Project( const Args& args ):
        name(                       args.name ),
        description(                args.description ),
        version(                    args.version ),
        authors(                    args.authors ),
        type(                       args.type ),
        compiler(                   args.compiler ),
        cpp_version(                args.cpp_version ),
        enable_cpp_extensions(      args.enable_cpp_extensions ),
        generate_compile_commands(  args.generate_compile_commands ),
        command_line_arguments(     args.command_line_arguments ),
        default_target(             args.default_target ),
        link_time_optimizations(    args.link_time_optimizations ),
        dependencies(               args.dependencies )
    #if defined( __APPLE__ )
        ,frameworks(                 args.frameworks )
    #endif
    {
        for( var &target: targets )
        {
            target.compiler_flags.insert(
                target.compiler_flags.end(),
                args.common_compiler_flags.begin(),
                args.common_compiler_flags.end() );

            target.defines.insert(
                target.defines.end(),
                args.common_defines.begin(),
                args.common_defines.end() );

            target.pre_build_scripts.insert(
                target.pre_build_scripts.end(),
                args.pre_build_scripts.begin(),
                args.pre_build_scripts.end() );

            target.post_build_scripts.insert(
                target.post_build_scripts.end(),
                args.post_build_scripts.begin(),
                args.post_build_scripts.end() );
        }
    }


    // TODO: Add more validation
    constexpr fun add_dependency( const Dependency& dependency )
    {
        REQUIRE( dependency.name != "UNNAMED" );
        REQUIRE( !dependency.name.empty() );

        let iter = std::ranges::find_if( dependencies, [ new_name = dependency.name ]( const Dependency& dep ) -> bool
        {
            return dep.name == new_name;
        });

        if( iter != dependencies.end() )
        {
            WARNING( "Dependency '{}' already exists. Skipping.", dependency.name );
            return;
        }
        dependencies.push_back( dependency );
    }

    // TODO: Support local frameworks
#if defined( __APPLE__ )
//{
    constexpr fun add_framework( const Framework& framework )
    {
        if constexpr( platform.is( Platform::Apple ) )
        {
            REQUIRE( framework.name != "UNNAMED" );
            REQUIRE( !framework.name.empty() );

            // This is just for the linker so I don't really care if it already exists
            frameworks.push_back( framework );
        }
    }
//}
#endif

    [[nodiscard]]
    constexpr fun find_target( const String& name ) const -> const Target*
    {
        let iter = std::ranges::find_if( targets, [ name ]( const Target& target ) -> bool
        {
            return strcasecmp( target.name.c_str(), name.c_str() ) == 0;
        });

        return iter == targets.end()
            ? nullptr
            : &(*iter);
    }

    [[nodiscard]]
    constexpr fun find_target( const String& name ) -> Target*
    {
        return const_cast<Target*>( as<const Project*>(this)->find_target(name) );
    }

    // TODO: Add more validation
    constexpr fun add_target( Target target )
    {
        REQUIRE( not target.name.empty() );

        to_lower( &target.name );

        REQUIRE( target.name != "unnamed" );
        REQUIRE_MSG( target.name != "all", "`All` is a reserved target name" );

        // We allow overwriting targets so people can add their own "debug" and "release"
        if( var* old_target = find_target( target.name ) )
        {
            *old_target = target;
        }
    }

    // Adds a define common to all targets
    constexpr fun add_define( const char* def )
    {
        for( var& target: targets )
            target.defines.emplace_back( def );
    }

    // Adds a compiler flag common to all targets
    constexpr fun add_compiler_flag( const char* arg )
    {
        for( var& target: targets )
            target.compiler_flags.emplace_back( arg );
    }

    // Adds extra flags to the linker for all targets
    constexpr fun add_linker_flag( const char* flag )
    {
        for( var& target: targets )
            target.linker_flags.emplace_back( flag );
    };

    // Adds a pre-build script common to all targets
    constexpr fun add_pre_build_script( const ScriptPtr& script )
    {
        for( var& target: targets )
            target.pre_build_scripts.push_back( script );
    }

    // Adds a post-build script common to all targets
    constexpr fun add_post_build_script( const ScriptPtr& script )
    {
        for( var& target: targets )
            target.post_build_scripts.push_back( script );
    }


    constexpr fun add_ignored_path( const char* path )
    {
        for( var& target: targets )
            target.ignored_paths.emplace_back( path );
    }


    constexpr fun set_link_time_optimizations( LTO mode )
    {
        if( mode == LTO::Incremental and this->compiler != Compiler::Clang )
        {
            WARNING( "Thin LTO is only supported in Clang. Full mode will be used instead." );
            mode = LTO::Full;
        }
        link_time_optimizations = mode;
    }


    String       name = "UNNAMED";
    String       description;
    Version      version;
    List<String> authors;
    Type         type;
    Compiler     compiler;
    u8           cpp_version;
    bool         enable_cpp_extensions;
    bool         generate_compile_commands;
    List<String> command_line_arguments;
    String       default_target; // `build` and `run` will use this target if none is provided
    LTO          link_time_optimizations;


    List<Target> targets
    {
        {
            .name               = "debug",
            .optimization_level = 0,
            .sanitizers         = ASan | UBSan,
            .defines            = { "DEBUG" },
            .compiler_flags     = {
                "g",
                "Wall",
                "Wextra",
                "Werror",
                "pedantic" }
        },
        {
            .name               = "test",
            .optimization_level = 0,
            .sanitizers         = ASan | UBSan,
            .defines            = { "TESTING" },
            .compiler_flags     = {
                "g",
                "Wall",
                "Wextra",
                "Werror",
                "pedantic" },
            .source_path        = "tests"
        },
        {
            .name               = "release",
            .optimization_level = 3,
            .sanitizers         = No_Sanitizers,
            .defines            = { "RELEASE" },
            .compiler_flags     = {
                "Wall",
                "Wextra",
                "Werror",
                "pedantic" }
        }
    };

    List<Dependency> dependencies{};

#if defined( __APPLE__ )
    List<Framework> frameworks{};
#endif
};


using CString = const char*;
struct MainArgvSlice
{
    String* data = nullptr;
    usize   size = 0;

    [[nodiscard]] constexpr fun begin() const -> const String* { return data; }
    [[nodiscard]] constexpr fun end()   const -> const String* { return data + size; }
};


#if !defined( INCLUDE_AS_HEADER )

#include <chrono>
#include <cstring>
using std::chrono::system_clock;
using Time = std::chrono::time_point<system_clock>;

#if defined( ENABLE_LOGS )

file_private fun fmt_time_since( const Time start ) -> String
{
    using std::chrono::minutes;
    using std::chrono::seconds;
    using std::chrono::milliseconds;

    let duration = system_clock::now() - start;
    let m  = duration_cast< minutes >( duration );
    let s  = duration_cast< seconds >( duration - m );
    let ms = duration_cast< milliseconds >( duration - m - s );

    return strfmt( "{}:{}:{}", m, s, ms );
}

#define TIME_SCOPE( scope_name )\
    let start_time = system_clock::now();\
    defer( INFO( "⏱️ {} took {}", scope_name, fmt_time_since( start_time ) ) )

#else // ENABLE_LOGS

#define TIME_SCOPE( scope_name )

#endif // ENABLE_LOGS


file_private fun is_directory_empty( const Path& path) -> bool
{
    if( FS::exists( path ) and FS::is_directory( path ) )
    {
        return FS::directory_iterator( path ) == FS::directory_iterator{};
    }
    return true;
}

enum [[nodiscard]] ResultCode: i16
{
    UNKNOWN_ERROR       = -1,
    OK                  = 0,
    // CLI errors
    INVALID_ARGUMENT    = EINVAL,
    INVALID_TARGET      = ELAST + 1,
    INVALID_GENERATOR,
    ALREADY_INITIALIZED,
    BUILD_SCRIPT_LOAD_FAILED,
    COMPILATION_FAILED,
    LINKING_FAILED,
    RUN_COMMAND_FAILED,
    MISSING_DEPENDENCY,
};


#if defined( ENABLE_LOGS )
file_private fun stringify_result( const ResultCode res ) -> String
{
    switch( res )
    {
        case UNKNOWN_ERROR:             return "UNKNOWN_ERROR";
        case OK:                        return "OK";
        case INVALID_ARGUMENT:          return "INVALID_ARGUMENT";
        case INVALID_TARGET:            return "INVALID_TARGET";
        case INVALID_GENERATOR:         return "INVALID_GENERATOR";
        case ALREADY_INITIALIZED:       return "ALREADY_INITIALIZED";
        case BUILD_SCRIPT_LOAD_FAILED:  return "BUILD_SCRIPT_LOAD_FAILED";
        case COMPILATION_FAILED:        return "COMPILATION_FAILED";
        case LINKING_FAILED:            return "LINKING_FAILED";
        case RUN_COMMAND_FAILED:        return "RUN_COMMAND_FAILED";
        case MISSING_DEPENDENCY:        return "MISSING_DEPENDENCY";
        default:                        return std::to_string( res );
    }
}
#endif // ENABLE_LOGS


// defer ///////////////////////////////////////////////////////////////////////////////////////////
template<typename Lambda>
class Deferrable
{
public:
    Deferrable() = delete;
    Deferrable( Lambda  f ): action( f ) {}
    ~Deferrable() { action(); }
private:
    const Lambda action;
};

// Preprocessor magic so __COUNTER__ can be expanded correctly
#define CONCATENATE( l, r )         DO_CONCATENATE( l, r )
#define DO_CONCATENATE( l, r )      DO_CONCATENATE_2( l, r )
#define DO_CONCATENATE_2( l, r )    l##r

#define defer( f ) const auto CONCATENATE( _deferred, __COUNTER__ ) = Deferrable( [&]() -> void { f; } );
////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr fun min( const auto a, const auto b ) ->  auto { return a < b ? a : b; }
constexpr fun max( const auto a, const auto b ) ->  auto { return a > b ? a : b; }

file_private fun trim( String* input )
{
    using std::ranges::find_if_not;
    using std::ranges::views::reverse;

    REQUIRE( input );
    // Left trim
    {
        let iter = find_if_not( *input, [](char c) -> bool { return isspace(c); } );
        input->erase( input->begin(), iter );
    }
    // right trim
    {
        let iter = find_if_not( reverse( *input ), [](char c) -> bool { return isspace(c); } );
        input->erase( iter.base(), input->end() );
    }
}


file_private fun split( const String& input, const char delimiter ) -> List<std::string_view>
{
    var slices = List<std::string_view>{};

    var start = 0u;
    while( start < input.size() )
    {
        var pos = input.find( delimiter, start );
        if( pos == std::string_view::npos )
            pos = input.size();

        slices.emplace_back( std::string_view( input ).substr( start, pos - start ) );
        start = pos + 1;
    }
    return slices;
}

#include <cstdio>
#include <dlfcn.h>

static let name_tag = String( "##NAME##" );

static let new_project_template = String(R"(
#define INCLUDE_AS_HEADER
#include "dedalo.cpp"
#undef INCLUDE_AS_HEADER

extern "C"
void build( Project* project, [[maybe_unused]] const MainArgvSlice args )
{
    REQUIRE( project );

    // **Target names are case-insensitive**
    // "Debug", "Test", and "Release" targets are provided by default.
    // You can override them by creating a new target with the same name.
    // "Debug" is the default target when none is provided to the `run` command.
    // C++20 is the default version.
    *project = Project(
    {
        .name = "##NAME##",
        .dependencies =
        {
            // You can add a system (dynamic) dependency like this:
            // { .name = "dependecy_name" }
        }
    });

    // Or you can add the dependencies after creating the project like this:
    // project->add_dependency({ .name = "dependecy_name" });

    // For static and/or local dependencies refer to the README, or just have a look at the code.

    // Any arguments after the target name are passed to the build script:
    // for( auto i = 0; i < args.size; ++i )
    // {
    //     const char* arg = args.data[ i ];
    //     // ...
    // }
}
)");

static let new_main_file_template = String(R"(
#include <stdio.h>

int main( [[maybe_unused]] int argc, [[maybe_unused]] char* argv[] )
{
    printf("Hello World!\n");
}
)");


// TODO: Validate the arguments
file_private let test_file_template_icaro = String(R"(
#include "../icaro.hpp"

auto main( int argc, char* argv[] ) -> int
{
    TEST( hello_tests,
    {
        VERIFY( true );
        return true;
    });

    Icaro::run({ .filter = argc > 1 ? argv[1] : "" });
}
)");


static let include_paths  = List<String>{ "src", "lib"  };
static let build_dir      = String( "./build"       );
static let dep_output_dir = String( "dep"       );
static let obj_output_dir = String( "obj"       );
static let bin_exec_dir   = String( "bin"       );
static let bin_lib_dir    = String( "bin/lib"   );
static let json_temp_dir  = String( "json"      );
static let lto_cache_dir  = String( "cache/lto" );
static let libraries_dir  = String( "./lib"             );


using BuildCfgFunPtr = void(*)( Project*, const MainArgvSlice args );
BuildCfgFunPtr build_cfg = nullptr;

var is_verbose = false;

template<typename Container, typename T>
constexpr fun contains( const Container& haystack, const T& needle ) -> bool
{
    return std::find( haystack.begin(), haystack.end(), needle ) != haystack.end();
}


fun init() -> ResultCode
{
    if( FS::is_regular_file( "build.cpp" ) )
    {
        WARNING( "Project already initialized. Skipping." );
        return ALREADY_INITIALIZED;
    }

    // Create the directory structure
    {
        FS::create_directory( "lib" );
        FS::create_directory( "tests" );
        // TODO: Create a cache and/or other build system files (ninja, make, CMake...)

        FS::create_directory( "src" );
        {
            var* main_file = fopen( "src/main.cpp", "w" );
            REQUIRE( main_file );
            fputs( new_main_file_template.c_str(), main_file );
            fclose( main_file );
        }
    }

    // Init build.cpp
    {
        var* build_file = fopen( "build.cpp", "w" );
        REQUIRE( build_file );

        // Replace the project's name with the current folder's
        let current_folder_name = FS::current_path().stem().string();
        var filled_template = new_project_template;
        filled_template.replace(
            filled_template.find( name_tag ),
            name_tag.length(),
            current_folder_name );

        fputs( filled_template.c_str(), build_file );
        fclose( build_file );
    }

    var* tests_file = fopen( "tests/tests.cpp", "w" );
    REQUIRE( tests_file );
    defer( fclose( tests_file ) );

    // TODO: Support other test frameworks
    if( FS::is_regular_file( "icaro.hpp" ) )
    {
        fputs( test_file_template_icaro.c_str(), tests_file );
    }

    return OK;
}


file_private fun gather_files(
    const Path&         in_path,
    const List<String>& extensions,
    const List<Path>&   excluded_paths,
          List<Path>*   gathered_paths )
{
    REQUIRE( gathered_paths );

    if( contains( excluded_paths, in_path ) )
        return;

    if( !FS::is_directory( in_path ) )
    {
        WARNING("Path '{}' is not a directory.", in_path.string() );
        return;
    }

    for( let &entry: FS::directory_iterator( in_path ) )
    {
        if( FS::is_directory( entry ) )
            gather_files( entry.path(), extensions, excluded_paths, gathered_paths ); // Recursion, yay!

        if( FS::is_regular_file( entry )
            and contains( extensions, entry.path().extension() )
            and !contains( excluded_paths, entry.path() ) )
        {
            // INFO( "File found: {}", entry.path().string() );
            gathered_paths->push_back( entry.path() );
        }
    }
}


static constexpr fun get_compiler_name( const Compiler compiler ) -> String
{
    switch (compiler)
    {
        case Compiler::Clang: return "clang++";
        case Compiler::GCC:   return "g++";
        case Compiler::MSVC:  UNIMPLEMENTED_MSG( "No support for MSVC yet" ); return "CL";
        default:              UNREACHABLE;
    }
}


static constexpr fun get_sanitizer_flags( const Target& target ) -> String
{
    var flags = String();

    if( target.sanitizers & ASan )
        flags += " -fsanitize=address";

    if( target.sanitizers & UBSan )
        flags += " -fsanitize=undefined";

    if( target.sanitizers & TSan )
        flags += " -fsanitize=thread";

    return flags;
}


static constexpr fun get_flags_from( const Target& target ) -> String
{
    var flags = String();
    for( let& flag: target.compiler_flags )
    {
        flags += " -" + flag;
    }
    return flags + get_sanitizer_flags( target );
}


static constexpr fun get_defines_from( const Target& target ) -> String
{
    var result = String();
    for( let& def: target.defines )
    {
        result += " -D" + def;
    }
    return result;
}


file_private fun needs_recompiling(
    const Path& obj_path,
    const Path& dep_path )
-> bool
{
    if( !FS::exists( obj_path ) )
    {
        return true;
    }

    if( var* obj_dep_file = fopen( dep_path.c_str(), "r" ) )
    {
        defer( fclose( obj_dep_file ) );

        let obj_timestamp = FS::last_write_time( obj_path );

        var  dummy         = usize( 0 );
        var  is_first_line = true;
        var* line          = as< char* >( malloc( 128 * sizeof( char ) ) );
        defer( free( line ) );

        while( ( getline( &line, &dummy, obj_dep_file ) ) > 0 )
        {
            String line_str;
            line_str.assign( line );

            trim( &line_str );
            let file_paths = split( line_str, ' ' );

            for( var i = 0u; i < file_paths.size(); ++i )
            {
                if( is_first_line and i == 0 )
                {
                    REQUIRE( file_paths.size() >= 2 );
                    REQUIRE( file_paths[0] == obj_path.string() + ":" );
                    is_first_line = false;
                    continue;
                }

                if( file_paths[i] == "\\" )
                    break; // EOL

                if( FS::last_write_time( file_paths[i] ) > obj_timestamp )
                    return true;
            }
        }
        return false;
    }
    return true;
}


file_private fun compile(
    const Project&    project,
    const Target&     target,
    const List<Path>& cpp_paths,
    const bool        force_compilation )
-> ResultCode
{
    INFO( "COMPILING..." );
    // TODO: Spread the compilation across multiple threads

    TIME_SCOPE( "Compilation phase" );

    struct ThreadCtx
    {
        Compiler compiler;
        String   compiler_flags;
        String   defines;
        String   target_name;
        String   source_path;
        String   include_paths;
        u8       cpp_version;
        u8       optimization_level;
        bool     generate_compile_commands;
        bool     force_compilation;
        LTO      link_time_optimizations;
    };

    var logging_mutex = std::mutex{};

    // TODO: Add logging
    let compile_task = [ &logging_mutex ](
        const ThreadCtx&   ctx,
        const List<Path>&  cpp_paths,
        const StopToken&   st,
              StopSrc      ss )
    -> void
    {

        for( let& source_file: cpp_paths )
        {
            if( st.stop_requested() )
                return;

            if( is_verbose )
            {
                // FIXME: Is this the best way to do this?
                let lock = MutexLock{ logging_mutex };
                INFO( "\t- {}", source_file.string() );
            }

            REQUIRE_MSG( source_file.is_relative(), "Something weird happened gathering the paths." );

            // FIXME: There must be a better way to do this. Perhaps with string views?
            var src_relative_cpp_path = Path();
            for( let& dir: source_file )
            {
                if( dir != Path( ctx.source_path ) )
                {
                    src_relative_cpp_path /= dir; // I hate this operator but the usage of `append` is even more awkward...
                }
            }

            let out_obj_path = Path( strfmt( "{}/{}/{}/{}.o", build_dir, ctx.target_name, obj_output_dir, src_relative_cpp_path.string() ) );
            let out_dep_path = Path( strfmt( "{}/{}/{}/{}.d", build_dir, ctx.target_name, dep_output_dir, src_relative_cpp_path.string() ) );
            let lto_path     = Path( strfmt( "{}/{}/{}",      build_dir, ctx.target_name, lto_cache_dir ) );

            // Make sure we're replicating the ./src tree in the obj and deps directories
            FS::create_directories( out_obj_path.parent_path() );
            FS::create_directories( out_dep_path.parent_path() );
            FS::create_directories( lto_path ); // FIXME: I think this is not used by MSCV

            if( !ctx.force_compilation and !needs_recompiling( out_obj_path, out_dep_path ) )
            {
                continue;
            }

            var command = strfmt(
                "{} -std=c++{} {} {} -O{} {} -c {} -o {} -MMD -MF {}",
                get_compiler_name( ctx.compiler ),
                ctx.cpp_version,
                ctx.compiler_flags,
                ctx.defines,
                ctx.optimization_level,
                ctx.include_paths,
                source_file.string(),
                out_obj_path.string(),
                out_dep_path.string() );

            if( ctx.generate_compile_commands )
            {
                // Make sure we're replicating the ./src tree in the compile_commands.json temp directory
                let out_json_path = Path( strfmt( "{}/{}/{}/{}.json", build_dir, ctx.target_name, json_temp_dir, src_relative_cpp_path.string() ) );
                FS::create_directories( out_json_path.parent_path() );
                command += strfmt( " -MJ {} ", out_json_path.string() );

                // TODO: Is this necessary on Windows too?
                if constexpr( platform.is( Platform::UNIX ) )
                {
                    // This is unnecessary for compilation, but clangd (the LSP not the compiler)
                    // doesn't seem to find system headers without it.
                    command += " -I/usr/local/include";
                }
            }

            if( ctx.link_time_optimizations != LTO::None )
            {
                switch( ctx.compiler )
                {
                    case Compiler::GCC:
                    {   command += " -lfto";
                        if( ctx.link_time_optimizations == LTO::Incremental )
                        {
                            // FIXME: I'm not entirely sure this is necessary
                            command += strfmt( " -lfto-incremental={}", lto_path.string() );
                        }
                        break;
                    }
                    case Compiler::Clang: command += ctx.link_time_optimizations == LTO::Incremental ? "-lfto=thin" : "-lfto=full"; break;
                    case Compiler::MSVC:  command += "/GL"; break;
                    default: UNREACHABLE;
                }
            }

            if( is_verbose )
            {
                // FIXME: Is this the best way to do this?
                let lock = MutexLock{ logging_mutex };
                INFO( "\t{}", command );
            }

            if( system( command.c_str() ) != OK )
            {
                ss.request_stop(); // Compilation has failed, stop all the other threads
            }
        }
    };

    var ctx_include_paths = String();
    for( let& path: include_paths )
    {
        ctx_include_paths += strfmt( "-I{} ", path );
    }
    if constexpr( platform.is( Platform::Apple ) )
    {
        ctx_include_paths += "-I/opt/homebrew/include";
    }

    // TODO: Spread the remaining files across all the threads rather than spawning a dedicated one
    let num_threads = min( Thread::hardware_concurrency(), cpp_paths.size() );
    let thread_ctx = ThreadCtx {
        .compiler                   = project.compiler,
        .compiler_flags             = get_flags_from( target ),
        .defines                    = get_defines_from( target ),
        .target_name                = target.name,
        .source_path                = target.source_path,
        .include_paths              = ctx_include_paths,
        .cpp_version                = project.cpp_version,
        .optimization_level         = target.optimization_level,
        .generate_compile_commands  = project.generate_compile_commands,
        .force_compilation          = force_compilation,
        .link_time_optimizations    = project.link_time_optimizations };

    INFO( "Compiling {} source files with {} threads:", cpp_paths.size(), num_threads );

    var thread_results  = List<ResultCode>{ num_threads, OK };
    var compile_threads = List<Thread>{};
    compile_threads.reserve( num_threads );

    let max_files_per_thread = as<u32>( std::ceil( as<f32>( cpp_paths.size() ) / as<f32>( num_threads ) ) );

    var stop_src = StopSrc{};
    for( var i = 0u; i < num_threads; ++i )
    {
        let start = i * max_files_per_thread;
        let end   = as<ptrdiff_t>( min( start + max_files_per_thread, cpp_paths.size() ) );
        REQUIRE( start < cpp_paths.size() );

        // Copy the thread's slice into a separated list to avoid false sharing
        let thread_paths = List< Path >{ cpp_paths.begin() + start, cpp_paths.begin() + end };

        compile_threads.emplace_back(
            compile_task,
            thread_ctx,
            thread_paths,
            stop_src.get_token(),
            stop_src );
    }

    for( var& thread: compile_threads )
    {
        thread.join();
    }

    return stop_src.stop_requested() ? COMPILATION_FAILED : OK;
}

// FIXME: This is probably doing too many allocations by concatenating strings
file_private fun link( const Project& project, const Target& target ) -> ResultCode
{
    INFO( "LINKING..." );

    TIME_SCOPE( "Linking phase" );

    let build_lib_dir = strfmt( "{}/{}/{}", build_dir, target.name, bin_lib_dir );
    FS::create_directories( build_lib_dir );

    var command = get_compiler_name( project.compiler );

    command += get_sanitizer_flags( target );

    // Add the compiled .o files
    {
        let obj_dir = strfmt( "{}/{}/{}", build_dir, target.name, obj_output_dir );
        var obj_paths = List<Path>();
        gather_files( obj_dir, {".o"}, {}, &obj_paths );

        REQUIRE_MSG( !obj_paths.empty(), "No compiled binaries to link?" );

        for( let& path: obj_paths )
        {
            command += " " + path.string();
        }
    }

    for( let& dependency: project.dependencies )
    {
        if( dependency.location == Location::Remote )
        {
            UNIMPLEMENTED_MSG( "No support for remote dependencies yet. Skipping {}.", dependency.name );
            continue;
        }

        // FIXME: Improve this lookup
        if( !contains(dependency.targets, "All" ) and !contains( dependency.targets, target.name ) )
            continue;

        let lib_dir = libraries_dir + "/" + dependency.name;
        var lib_files = List<Path>{}; // A single dependency might have multiple binary files that need to be linked
        let excluded_subdirs = List<Path>{}; // TODO: Automatically exclude paths based on platform?

        // TODO: Include the version somehow
        switch( dependency.linking )
        {
            case Linking::Static:
            {
                REQUIRE( FS::is_directory( lib_dir ) );
                gather_files(
                    lib_dir,
                    { platform.static_lib_extension() },
                    excluded_subdirs,
                    &lib_files );
                break;
            }
            case Linking::Dynamic:
            {
                switch( dependency.location )
                {
                    case Location::System:
                    {
                        command += " -l" + dependency.name; // system library
                        break;
                    }
                    case Location::Local:
                    {
                        REQUIRE( FS::is_directory( lib_dir ) );

                        gather_files(
                            lib_dir,
                            { platform.dynamic_lib_extension() },
                            excluded_subdirs,
                            &lib_files );

                        // So they can be loaded correctly at runtime
                        for( let& lib: lib_files )
                        {
                            FS::copy_file(
                                lib,
                                build_lib_dir + "/" + lib.filename().string(),
                                FS::copy_options::overwrite_existing );
                        }
                        break;
                    }
                    case Location::Remote:
                    {
                        UNIMPLEMENTED_MSG( "Remote dynamic libraries are not supported." );
                        break;
                    }
                }
                break;
            }
            case Linking::SingleHeader:
            {
                // Nothing to link
                break;
            }
        }

        for( let& lib: lib_files )
        {
            command += " " + lib.string();
        }
        command += " " + dependency.linker_flags;
    }

    for( let& flag: target.linker_flags )
    {
        command += " -" + flag;
    }

    #if defined( __APPLE__ )
    {
        for( let& framework: project.frameworks )
        {
            // FIXME: Improve this lookup
            if( !contains( framework.targets, "All" ) and !contains( framework.targets, target.name ) )
                continue;

            command += " -framework " + framework.name;
        }
        // The runtime doesn't seem to find system libraries stored in `/usr/local/lib`
        // (despite them linking fine) because that path is not in the @rpath.
        command += " -Wl,-rpath,/usr/local/lib";
        command += " -L/opt/homebrew/lib";
    }
    #endif

    command += " -L" + build_lib_dir;

    if( project.link_time_optimizations != LTO::None )
    {
        switch( project.compiler )
        {
            case Compiler::GCC:
            {
                command += " -lfto";
                if( project.link_time_optimizations == LTO::Incremental )
                {
                    command += strfmt( " -lfto-incremental={}/{}/{}", build_dir, target.name, lto_cache_dir );
                }
                break;
            }
            case Compiler::Clang:
            {
                if( project.link_time_optimizations == LTO::Incremental )
                {
                    // TODO: Add a cache path (atm not working on macOS for some reason)
                    command += " -flto=thin";
                }
                else
                {
                    command += "-lfto=full";
                }
                break;
            }
            case Compiler::MSVC:  command += project.link_time_optimizations == LTO::Incremental ? "/LTGC:INCREMENTAL" : "/LTGC";      break;
            default: UNREACHABLE;
        }
    }

    command += strfmt( " -o {}/{}/{}/{}", build_dir, target.name, bin_exec_dir, project.name );

    INFO( "{}", command );

    if( system( command.c_str() ) != OK )
    {
        ERROR( "Failed linking.\nCommand:\n\t{}", command );
        return LINKING_FAILED;
    }
    return OK;
}


file_private fun compile_config( bool* has_changed ) -> ResultCode
{
    REQUIRE( has_changed );

    TIME_SCOPE( "Compiling the build config" );

    constant so_path  = "./build/build_script.so";
    constant cpp_path = "./build.cpp";

    if( FS::exists( so_path ) and FS::last_write_time( so_path ) >= FS::last_write_time( cpp_path ) )
    {
        *has_changed = false;
        return OK; // The binary is up to date, no need to recompile
    }
    *has_changed = true;

    INFO( "Compiling build config..." );
    // TODO: Dynamically choose the compiler
    if( let error = system( "clang++ --std=c++20 -fPIC -shared -o ./build/build_script.so build.cpp" ) )
    {
        ERROR( "Build script compilation failed with error {}.", error );
        return BUILD_SCRIPT_LOAD_FAILED;
    }
    return OK;
}


file_private fun build_compile_commands_json( const String& target_name )
{
    let build_json_dir = strfmt( "{}/{}/{}", build_dir, target_name, json_temp_dir );
    REQUIRE( FS::is_directory( json_temp_dir ) );

    var json_paths = List<Path>{};
    gather_files( build_json_dir, {".json"}, {}, &json_paths );

    var* cmp_cmd_json = fopen( "compile_commands.json", "w" );
    REQUIRE( cmp_cmd_json );
    defer( fclose( cmp_cmd_json ) );

    fputs( "[\n", cmp_cmd_json );
    defer( fputs( "]", cmp_cmd_json ) );

    for( let& entry: json_paths )
    {
        var* part = fopen( entry.c_str(), "r" );
        defer( fclose( part ) );

        // The output of -MJ for a single source file should be a single line json file
        var line_len = usize( 0 );
        char* line = nullptr;
        defer( free( line ) );

        getline( &line, &line_len, part );
        REQUIRE( line );
        REQUIRE( line_len > 0 );

        fputs( line, cmp_cmd_json );
    }
}


file_private fun build( String target_name, const bool run_after_build, const MainArgvSlice args ) -> ResultCode
{
    FS::create_directories( build_dir );

    bool config_recompiled = false;
    if( let error = compile_config( &config_recompiled ) )
    {
        return error;
    }

    INFO( "Loading build config..." );
    {
        var* dll = dlopen( "./build/build_script.so", RTLD_NOW );
        if( !dll )
        {
            ERROR( "Couldn't load build script's DLL." );
            return BUILD_SCRIPT_LOAD_FAILED;
        }
        build_cfg = (BuildCfgFunPtr) dlsym(dll, "build");
        if( !build_cfg )
        {
            ERROR( "Couldn't load build function's symbol." );
            return BUILD_SCRIPT_LOAD_FAILED;
        }
        // No need to close the dll, the OS will clean up after us.
    }

    var project = Project();
    build_cfg( &project, args );
    REQUIRE( project.name != "UNNAMED" );

    if( project.compiler != Compiler::Clang and project.generate_compile_commands )
    {
        UNIMPLEMENTED_MSG( "No support for generating compile_commands.json with compilers other than Clang" );
        project.generate_compile_commands = false;
    }

    // Check that SingleHeader dependencies' directories are where they should and not empty
    for( let& dependency: project.dependencies )
    {
        if( dependency.linking == Linking::SingleHeader
            and is_directory_empty( libraries_dir + "/" + dependency.name ) )
        {
            ERROR( "Couldn't find single-header dependency '{}' in ./lib", dependency.name );
            return MISSING_DEPENDENCY;
        }
    }

    if( target_name.empty() )
        target_name = project.default_target;

    to_lower( &target_name );

    if( let* target = project.find_target( target_name ) )
    {
        INFO( "Starting build of project \"{}\" for target \"{}\"...", project.name, target->name );
        {
            TIME_SCOPE( "Building phase" );

            // Make sure the build directories exist
            FS::create_directories( strfmt( "{}/{}/{}", build_dir, target_name, dep_output_dir ) );
            FS::create_directories( strfmt( "{}/{}/{}", build_dir, target_name, obj_output_dir ) );

            for( var i = 0u; i < target->pre_build_scripts.size(); ++i )
            {
                let script = target->pre_build_scripts[i];
                REQUIRE( script.func != nullptr );

                TIME_SCOPE( script.name );

                INFO( "Running pre-build script #{}: '{}' ...", i+1, script.name );
                if( script.func() == false )
                {
                    ERROR( "Pre-build script #{} failed!", i );
                }
            }

            // Find all the source files
            var cpp_paths = List<Path>{};
            gather_files( target->source_path, {".cpp", ".cc", ".cxx"}, target->ignored_paths, &cpp_paths );

            // TODO: Fetch any remote dependencies and place them in lib/
            // TODO: Link any dynamic libraries into their corresponding .so in build/bin/

            if( let error = compile( project, *target, cpp_paths, config_recompiled ) )
            {
                return error;
            }
            if( project.generate_compile_commands )
            {
                build_compile_commands_json( target->name );
            }
            if( let error = link( project, *target ) )
            {
                return error;
            }

            for( var i = 0u; i < target->post_build_scripts.size(); ++i )
            {
                let script = target->post_build_scripts[i];
                REQUIRE( script.func != nullptr );

                TIME_SCOPE( script.name );

                INFO( "Running post-build script #{}: '{}'...", i+1, script.name );
                if( script.func() == false )
                {
                    ERROR( "Post-build script #{} failed!", i );
                }
            }
        }

        if( run_after_build )
        {
            INFO( "RUNNING...\n" );
            var command = strfmt( "./{}/{}/{}/{}", build_dir, target->name, bin_exec_dir, project.name );

            if( target_name == "test" )
            {
                for( let &arg: args )
                {
                    command += " " + arg;
                }
            }

            if( system( command.c_str() ) != OK )
            {
                return RUN_COMMAND_FAILED;
            }
        }
        return OK;
    }
    else
    {
        ERROR( "Target '{}' doesn't exist.", target_name );
        return INVALID_TARGET;
    }
}


fun clean() -> ResultCode
{
    FS::remove_all( "./build" );
    return OK;
}


enum struct Command : u8 { Init, Build, Run, Test, Clean, Version, Unknown };
file_private fun parse_command( String cmd ) -> Command
{
    to_lower( &cmd );

    if( cmd == "build" )
        return Command::Build;
    else if( cmd == "run" )
        return Command::Run;
    else if( cmd == "test" )
        return Command::Test;
    else if( cmd == "clean" )
        return Command::Clean;
    else if( cmd == "version" )
        return Command::Version;
    else if( cmd == "init" )
        return Command::Init;
    else
        return Command::Unknown;
}


file_private fun print_valid_commands()
{
    println( stderr,
        "Valid commands:\n"
            "\tbuild\n"
            "\tclean\n"
            "\tinit\n"
            "\trun\n"
            "\ttest\n"
            "\tversion" );
}


fun main( i32 argc, char* argv[] ) -> i32
{
    if( argc < 2 )
    {
        ERROR( "No command provided." );
        print_valid_commands();
        return INVALID_ARGUMENT;
    }


    var arguments = List<String>{};
    arguments.reserve( argc - 1 );

    for( var i = 1; i < argc; ++i )
    {
        // Parse flags
        if( argv[i][0] == '-' )
        {
            if( strcmp( argv[i], "-v" ) == 0  or strcmp( argv[i], "--verbose" ) == 0 )
            {
                is_verbose = true;
            }
            continue;
        }
        arguments.emplace_back( argv[i] );
    }

    let cmd = parse_command( arguments[0] );
    switch( cmd )
    {
        case Command::Init:
            return init();
        case Command::Clean:
            return clean();
        case Command::Version:
            println( stdout, "Dedalo v{}.{}.{}", version.major, version.minor, version.patch );
            return OK;
        case Command::Build:
        case Command::Run:
        case Command::Test:
        {
            var target_name = String("");
            let run_after_build = ( cmd == Command::Run or cmd == Command::Test );

            // Pass the remaining args to the build script
            var first_remaining_arg = 1u;
            if( cmd == Command::Test )
            {
                target_name = "Test";
            }
            else if( arguments.size() > 1 )
            {
                target_name = arguments[1];
                first_remaining_arg += 1;

            }

            REQUIRE( first_remaining_arg <= arguments.size() );

            let remaining_args = MainArgvSlice{
                .data = arguments.data() + first_remaining_arg,
                .size = arguments.size() - first_remaining_arg };

            if( let error = build( target_name, run_after_build, remaining_args ) )
            {
                ERROR( "Build failed with error {}", stringify_result( error ) );
                return error;
            }
            return OK;
        }
        default:
        {
            ERROR( "Unrecognized command '{}'.", arguments[0] );
            print_valid_commands();
            return INVALID_ARGUMENT;
        }
    }
}

#endif // !INCLUDE_AS_HEADER

