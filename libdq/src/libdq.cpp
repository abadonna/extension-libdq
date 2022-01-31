// myextension.cpp
// Extension lib defines
#define LIB_NAME "libdq"
#define MODULE_NAME "libdq"

// include the Defold SDK
#include <dmsdk/sdk.h>
#include <dq.h>
#include <dq_mat3.h>
#include <dq_homo.h>

//================================================================================================

static int luaA_typerror( lua_State *L, int narg, const char *tname )
{
    const char *msg = lua_pushfstring(L, "%s expected, got %s",
    tname, luaL_typename(L, narg));
    return luaL_argerror(L, narg, msg);
}


/*
 * DQ API
 */
typedef struct dqL_s {
   dq_t dq;
} dqL_t;
#define DQ_METATABLE    "luadq"

/*
 * Generic.
 */
static int lua_reg_metatable( lua_State *L, const char *name )
{
    lua_createtable(     L, 0, 0 );
    luaL_newmetatable(   L, name );
 
    lua_setfield(        L, -2, "__index" );
    lua_setmetatable(    L, -2 );
   
    return 0;
}
static int lua_is_foo( lua_State *L, int ind, const char *foo )
{
   int ret;
   if (lua_getmetatable(L,ind)==0)
      return 0;
   lua_getfield(L, LUA_REGISTRYINDEX, foo);
   ret = 0;
   if (lua_rawequal(L, -1, -2)) /* does it have the correct mt? */
      ret = 1;
   lua_pop(L, 2); /* remove both metatables */
   return ret;
}


/*
 * Helper.
 */
#define TBL_GETNUM(L,v,n,ind) \
lua_pushnumber(L,n); \
lua_gettable(L,ind); \
v = luaL_checknumber(L,-1); \
lua_pop(L,1)
static int luaL_checkvec3( lua_State *L, double v[3], int ind )
{
   int i;
   luaL_checktype(L,ind,LUA_TTABLE);
   for (i=0; i<3; i++) {
      TBL_GETNUM( L, v[i], i+1, ind );
   }
   return 0;
}
static int luaL_checkmatrix( lua_State *L, double *M, int rows, int columns, int ind )
{
   int i, j;
   luaL_checktype(L,ind,LUA_TTABLE);
   for (i=0; i<rows; i++) {
      /* Get row. */
      lua_pushnumber(L,i+1);
      lua_gettable(L,ind);
      luaL_checktype(L,-1,LUA_TTABLE);

      /* Process row. */
      for (j=0; j<columns; j++) {
         TBL_GETNUM( L, M[i*rows+j], j+1, -2 );
      }

      /* Clean up. */
      lua_pop(L,1);
   }
   return 0;
}
static int luaL_checkmat3( lua_State *L, double M[3][3], int ind )
{
   return luaL_checkmatrix( L, (double*)M, 3, 3, ind );
}
static int lua_pushvec3( lua_State *L, const double v[3] )
{
   int i;
   lua_newtable(L);
   for (i=0; i<3; i++) {
      lua_pushnumber( L, i+1 );
      lua_pushnumber( L, v[i] );
      lua_settable( L, -3 );
   }
   return 0;
}
static int lua_pushmat3( lua_State *L, double M[3][3] )
{
   int i, j;
   lua_newtable(L);
   for (j=0; j<3; j++) {
      lua_pushnumber( L, j+1 );
      lua_newtable(L);
      for (i=0; i<3; i++) {
         lua_pushnumber( L, i+1 );
         lua_pushnumber( L, M[j][i] );
         lua_settable( L, -3 );
      }
      lua_settable( L, -3 );
   }
   return 0;
}


/*
 * DQ.
 */
/* Internal API. */

static int lua_isdq( lua_State *L, int ind )
{
   return lua_is_foo( L, ind, DQ_METATABLE );
}
static dqL_t* lua_todq( lua_State *L, int ind )
{
   return (dqL_t*) lua_touserdata( L, ind );
}
static dqL_t* luaL_checkdq( lua_State *L, int ind )
{
   if (lua_isdq( L, ind ))
      return lua_todq( L, ind );
   luaA_typerror( L, ind, DQ_METATABLE );
   return NULL;
}
static dqL_t* lua_pushdq( lua_State *L, dqL_t dq )
{
   dqL_t *dqL;
   dqL = (dqL_t*) lua_newuserdata( L, sizeof(dqL_t) );
   *dqL = dq;
   assert( dqL != NULL );
   luaL_getmetatable( L, DQ_METATABLE );
   lua_setmetatable( L, -2 );
   return dqL;
}
/* Creation API. */
#define DQL_CR_V( n ) \
static int dqL_##n( lua_State *L ) { \
   dqL_t Q; double v[3]; \
   luaL_checkvec3( L, v, 1 ); \
   dq_##n( Q.dq, v ); \
   lua_pushdq( L, Q ); \
   return 1; }
#define DQL_CR_VV( n ) \
static int dqL_##n( lua_State *L ) { \
   dqL_t Q; double v[3], u[3]; \
   luaL_checkvec3( L, v, 1 ); \
   luaL_checkvec3( L, u, 2 ); \
   dq_##n( Q.dq, v, u ); \
   lua_pushdq( L, Q ); \
   return 1; }
#define DQL_CR_DV( n ) \
static int dqL_##n( lua_State *L ) { \
   dqL_t Q; double d, v[3]; \
   d = luaL_checknumber( L, 1 ); \
   luaL_checkvec3( L, v, 2 ); \
   dq_##n( Q.dq, d, v ); \
   lua_pushdq( L, Q ); \
   return 1; }
#define DQL_CR_DVV( n ) \
static int dqL_##n( lua_State *L ) { \
   dqL_t Q; double d, v[3], u[3]; \
   d = luaL_checknumber( L, 1 ); \
   luaL_checkvec3( L, v, 2 ); \
   luaL_checkvec3( L, u, 3 ); \
   dq_##n( Q.dq, d, v, u ); \
   lua_pushdq( L, Q ); \
   return 1; }
#define DQL_CR_M( n ) \
static int dqL_##n( lua_State *L ) { \
   dqL_t Q; double M[3][3]; \
   luaL_checkmat3( L, M, 1 ); \
   dq_##n( Q.dq, M ); \
   lua_pushdq( L, Q ); \
   return 1; }
#define DQL_CR_MV( n ) \
static int dqL_##n( lua_State *L ) { \
   dqL_t Q; double M[3][3], v[3]; \
   luaL_checkmat3( L, M, 1 ); \
   luaL_checkvec3( L, v, 2 ); \
   dq_##n( Q.dq, M, v ); \
   lua_pushdq( L, Q ); \
   return 1; }
#define DQL_CR_Q( n ) \
static int dqL_##n( lua_State *L ) { \
   dqL_t Q; dqL_t *P; \
   P = luaL_checkdq( L, 1 ); \
   dq_##n( Q.dq, P->dq ); \
   lua_pushdq( L, Q ); \
   return 1; }
static int dqL_cr_raw( lua_State *L )
{
   int i;
   dqL_t Q;
   luaL_checktype(L,1,LUA_TTABLE);
   for (i=0; i<8; i++) {
      TBL_GETNUM( L, Q.dq[i], i+1, 1 );
   }
   lua_pushdq( L, Q );
   return 1;
}
DQL_CR_DVV( cr_rotation )
DQL_CR_DVV( cr_rotation_plucker )
DQL_CR_M(   cr_rotation_matrix )
DQL_CR_DV(  cr_translation )
DQL_CR_V(   cr_translation_vector )
DQL_CR_V(   cr_point )
DQL_CR_VV(  cr_line )
DQL_CR_VV(  cr_line_plucker )
DQL_CR_MV(  cr_homo )
DQL_CR_Q(   cr_copy )
DQL_CR_Q(   cr_conj )
DQL_CR_Q(   cr_inv )
/* Operations. */
static int dqL_op_norm2( lua_State *L )
{
   double real, dual;
   dqL_t *Q;
   Q = luaL_checkdq( L, 1 );
   dq_op_norm2( &real, &dual, Q->dq );
   lua_pushnumber( L, real );
   lua_pushnumber( L, dual );
   return 2;
}
#define DQL_OP_Q( n ) \
static int dqL_##n( lua_State *L ) { \
   dqL_t O, *Q; \
   Q = luaL_checkdq( L, 1 ); \
   dq_##n( O.dq, Q->dq ); \
   lua_pushdq( L, O ); \
   return 1; }
#define DQL_OP_QQ( n ) \
static int dqL_##n( lua_State *L ) { \
   dqL_t O, *Q, *P; \
   Q = luaL_checkdq( L, 1 ); \
   P = luaL_checkdq( L, 2 ); \
   dq_##n( O.dq, Q->dq, P->dq ); \
   lua_pushdq( L, O ); \
   return 1; }
DQL_OP_QQ(  op_add )
DQL_OP_QQ(  op_sub )
DQL_OP_QQ(  op_mul )
DQL_OP_Q(   op_sign )
DQL_OP_QQ(  op_f1g )
DQL_OP_QQ(  op_f2g )
DQL_OP_QQ(  op_f3g )
DQL_OP_QQ(  op_f4g )
static int dqL_op_extract( lua_State *L )
{
   dqL_t *Q;
   double R[3][3], d[3];
   Q = luaL_checkdq( L, 1 );
   dq_op_extract( R, d, Q->dq );
   lua_pushmat3( L, R );
   lua_pushvec3( L, d );
   return 2;
}
/* Check stuff. */
static int dqL_ch_get( lua_State *L )
{
   int i;
   dqL_t *Q = luaL_checkdq( L, 1 );
   lua_newtable(L);
   for (i=0; i<8; i++) {
      lua_pushnumber( L, i+1 );
      lua_pushnumber( L, Q->dq[i] );
      lua_settable( L, -3 );
   }
   return 1;
}
static int dqL_ch_unit( lua_State *L )
{
   dqL_t *Q = luaL_checkdq( L, 1 );
   lua_pushboolean( L, dq_ch_unit( Q->dq ) );
   return 1;
}
static int dqL_ch_cmp( lua_State *L )
{
   dqL_t *P, *Q;
   double precision;
   P = luaL_checkdq( L, 1 );
   Q = luaL_checkdq( L, 2 );
   precision = DQ_PRECISION;
   if (lua_gettop(L) > 2)
      precision = luaL_checknumber( L, 3 );
   lua_pushboolean( L, !dq_ch_cmpV( P->dq, Q->dq, precision ) );
   return 1;
}
/* Misc. */
static int dqL_print( lua_State *L )
{
   dqL_t *Q = luaL_checkdq( L, 1 );
   int vert = lua_toboolean( L, 2 );
   if (vert)
      dq_print_vert( Q->dq );
   else
      dq_print( Q->dq );
   return 0;
}
static int dqL_version( lua_State *L )
{
   int major, minor;
   dq_version( &major, &minor );
   lua_pushinteger( L, major );
   lua_pushinteger( L, minor );
   return 2;
}

//================================================================================================

static int Test(lua_State* L)
{
    dq_t Q;
    double t[3] = { 1.0, 2.0, 3.0 };
    dq_cr_point( Q, t );
    dq_print_vert( Q );
    
    return 0;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    
    /* Creation. */
    { "raw", dqL_cr_raw },
    { "rotation", dqL_cr_rotation },
    { "rotation_plucker", dqL_cr_rotation_plucker },
    { "rotation_matrix", dqL_cr_rotation_matrix },
    { "translation", dqL_cr_translation },
    { "translation_vector", dqL_cr_translation_vector },
    { "point", dqL_cr_point },
    { "line", dqL_cr_line },
    { "line_plucker", dqL_cr_line_plucker },
    { "homo", dqL_cr_homo },
    { "copy", dqL_cr_copy },
    { "conj", dqL_cr_conj },
    { "inv", dqL_cr_inv },
    /* Operation. */
    { "norm2", dqL_op_norm2 },
    { "add", dqL_op_add },
    { "__add", dqL_op_add },
    { "sub", dqL_op_sub },
    { "__sub", dqL_op_sub },
    { "mul", dqL_op_mul },
    { "__mul", dqL_op_mul },
    { "sign", dqL_op_sign },
    { "f1g", dqL_op_f1g },
    { "f2g", dqL_op_f2g },
    { "f3g", dqL_op_f3g },
    { "f4g", dqL_op_f4g },
    { "extract", dqL_op_extract },
    /* Check. */
    { "get", dqL_ch_get },
    { "unit", dqL_ch_unit },
    { "cmp", dqL_ch_cmp },
    /* Misc. */
    { "print", dqL_print },
    { "version", dqL_version },
    {"test", Test},
    {0, 0}
};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, MODULE_NAME, Module_methods);
    lua_reg_metatable(L, DQ_METATABLE);
    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

static dmExtension::Result AppInitializeMyExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppInitializeMyExtension");
    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializeMyExtension(dmExtension::Params* params)
{
    // Init Lua
    LuaInit(params->m_L);
    dmLogInfo("Registered %s Extension", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeMyExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppFinalizeMyExtension");
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeMyExtension(dmExtension::Params* params)
{
    dmLogInfo("FinalizeMyExtension");
    return dmExtension::RESULT_OK;
}

static dmExtension::Result OnUpdateMyExtension(dmExtension::Params* params)
{
    dmLogInfo("OnUpdateMyExtension");
    return dmExtension::RESULT_OK;
}

static void OnEventMyExtension(dmExtension::Params* params, const dmExtension::Event* event)
{
    switch(event->m_Event)
    {
        case dmExtension::EVENT_ID_ACTIVATEAPP:
            dmLogInfo("OnEventMyExtension - EVENT_ID_ACTIVATEAPP");
            break;
        case dmExtension::EVENT_ID_DEACTIVATEAPP:
            dmLogInfo("OnEventMyExtension - EVENT_ID_DEACTIVATEAPP");
            break;
        case dmExtension::EVENT_ID_ICONIFYAPP:
            dmLogInfo("OnEventMyExtension - EVENT_ID_ICONIFYAPP");
            break;
        case dmExtension::EVENT_ID_DEICONIFYAPP:
            dmLogInfo("OnEventMyExtension - EVENT_ID_DEICONIFYAPP");
            break;
        default:
            dmLogWarning("OnEventMyExtension - Unknown event id");
            break;
    }
}

// Defold SDK uses a macro for setting up extension entry points:
//
// DM_DECLARE_EXTENSION(symbol, name, app_init, app_final, init, update, on_event, final)

// MyExtension is the C++ symbol that holds all relevant extension data.
// It must match the name field in the `ext.manifest`
DM_DECLARE_EXTENSION(libdq, LIB_NAME, 0, 0, InitializeMyExtension, 0, 0, FinalizeMyExtension)
