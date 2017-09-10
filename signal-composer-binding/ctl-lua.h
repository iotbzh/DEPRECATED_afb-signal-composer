#ifdef SUSE_LUA_INCDIR
    #include <lua5.3/lua.h>
    #include <lua5.3/lauxlib.h>
    #include <lua5.3/lualib.h>
#else
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
#endif

#include <json-c/json.h>

#include "signal-composer-binding.hpp"

typedef enum {
    LUA_DOCALL,
    LUA_DOSTRING,
    LUA_DOSCRIPT,
} LuaDoActionT;

typedef int (*Lua2cFunctionT)(char *funcname, json_object *argsJ, void*context);

typedef int (*Lua2cWrapperT) (lua_State* luaState, char *funcname, Lua2cFunctionT callback);

int LuaLibInit ();
void LuaL2cNewLib(const char *label, luaL_Reg *l2cFunc, int count);
int Lua2cWrapper(lua_State* luaState, char *funcname, Lua2cFunctionT callback, void *context);
int LuaCallFunc (DispatchSourceT source, DispatchActionT *action, json_object *queryJ) ;
