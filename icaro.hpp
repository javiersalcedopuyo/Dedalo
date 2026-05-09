#pragma once

#if __cplusplus < 202002L
    #error Unsupported C++ version. C++20 or newer is required.
#endif

#include <cstdio>
#include <format>

#define TEST( name, test ) Icaro::tests.emplace_back( #name, []( [[maybe_unused]] void* ctx )-> bool { test }  );

// NOTE:
//  For specific types like optionals or result/expected unions, you'll need to define your own
//  template specialisations for ImplVerify
#define VERIFY( condition )     if( !Icaro::ImplVerify( (condition), #condition ) ) return false;
#define VERIFY_EQ( a, b )       if( !Icaro::ImplVerifyEq( (a), (b), #a, #b ) )      return false;
#define VERIFY_NOT_EQ( a, b )   if( !Icaro::ImplVerifyNotEq( (a), (b), #a, #b ) )   return false;
#define VERIFY_PTR( ptr )       if( !(ptr) ){ Icaro::println( "❌ FAILED: Expected '{}' to be not NULL", #ptr ); return false; }


namespace Icaro
{
    struct Version{ int major, minor, patch; };
    inline constexpr Version version{ .major = 0, .minor = 1, .patch = 1 };

    using String = const char*;

    [[ nodiscard ]]
    static constexpr auto strempty( const char* str ) -> bool
    {
        return str == nullptr or str[0] == '\0';
    }

    template< typename T >
    class List
    {
    public:
        List() = default;
        ~List() { free( static_cast<void*>( _data ) ); }

        [[ nodiscard ]]
        constexpr auto size() const -> size_t { return _size; }

        auto reserve( const size_t new_capacity ) -> bool
        {
            if( new_capacity <= _capacity and _capacity > 0 )
                return true;

            _data = static_cast<T*>( _data == nullptr
                ?  malloc( new_capacity * sizeof( T ) )
                : realloc( static_cast<void*>( _data ), new_capacity * sizeof( T ) ) );

            _capacity = new_capacity;

            return _data != nullptr;
        }

        template< typename ...Args >
        auto emplace_back( Args&&... args ) -> bool
        {
            if( _size >= _capacity )
            {
                if( not reserve( _capacity == 0 ? 1 : _capacity * 2 ) )
                {
                    return false;
                }
            }

            new ( static_cast<void*>( _data + _size ) ) T{ std::forward<Args>( args )... };
            _size += 1;

            return true;
        }

        [[ nodiscard ]] constexpr auto begin()       ->       T* { return _data;         }
        [[ nodiscard ]] constexpr auto begin() const -> const T* { return _data;         }
        [[ nodiscard ]] constexpr auto end()         ->       T* { return _data + _size; }
        [[ nodiscard ]] constexpr auto end()   const -> const T* { return _data + _size; }

    private:
        T*     _data     = nullptr;
        size_t _size     = 0;
        size_t _capacity = 0;
    };

    template< typename... Args >
    static void println( std::format_string< Args... > fmt, Args&&... args )
    {
        printf( "%s\n", std::format( fmt, std::forward< Args >( args )... ).c_str() );
    }

    struct Test
    {
        Test() = delete;
        Test( const char* _name, bool (*lambda)(void* ctx) ): name( _name ), test_fun( lambda ), context( nullptr ) {}

        // TODO: Print how long it took
        [[ nodiscard ]]
        auto run() const -> bool
        {
            println( "🧪 {}:", name );
            if( test_fun == nullptr )
            {
                println( "❌ FAILED: No test with this name!" );
                println( "---" );
                return false;
            }
            const auto passed = test_fun( context );
            if( passed )
            {
                println( "✅ PASSED" );
            }
            return passed;
        }

        String name;
        bool (*test_fun)(void* ctx);
        void* context; // For `setup` to populate
    };


    // TODO: Group these into test suites
    static List< Test > tests{};

    struct RunArgs
    {
        String filter       = "";
        void (*setup)( void** context )     = nullptr;
        void (*teardown)( void** context )  = nullptr;
    };
    static void run( const RunArgs& args )
    {
        auto passed = 0u;
        auto failed = List<String>{};
        failed.reserve( tests.size() );

        for( auto& test: tests )
        {
            if( strempty( args.filter )
                or strcasestr( test.name, args.filter ) )
            {
                if( args.setup )
                    args.setup( &test.context );

                if( test.run() )
                    ++passed;
                else
                    failed.emplace_back( test.name );

                if( args.teardown )
                    args.teardown( &test.context );

                println( "---" );
            }
        }

        // TODO: Print how long it took
        println( "🏁 TEST RESULTS:" );
        if( not strempty( args.filter ) )
        {
            println( "\tℹ️ Tests ran with filter: '{}'.", args.filter );
        }
        println( "\t📋 {}/{} tests run.", passed + failed.size(), tests.size() );
        println( "\t✅ {} tests passed.", passed );
        println( "\t❌ {} tests failed.", failed.size() );

        for( const auto& name: failed )
            println( "\t\t- {}", name );
    }


    template< typename T >
    constexpr auto ImplVerify( const T& condition, const String& text ) -> bool
    {
        if( condition == false )
        {
            println( "❌ FAILED: Expected '{}' to be true.", text );
            return false;
        }
        return true;
    }


    template<typename A, typename B>
    constexpr auto ImplVerifyEq( const A& a, const B& b, const String& a_text, const String& b_text ) -> bool
    {
        if( a == b ) return true;

        println( "❌ FAILED: Expected '{} == {}'", a_text, b_text );
        println( "           Found: {}, and {}", a, b );
        return false;
    }


    template<typename A, typename B>
    constexpr auto ImplVerifyNotEq( const A& a, const B& b, const String& a_text, const String& b_text ) -> bool
    {
        if( a != b ) return true;

        println( "❌ FAILED: Expected '{} != {}'", a_text, b_text );
        println( "           Found: {}, and {}", a, b );
        return false;
    }
}
