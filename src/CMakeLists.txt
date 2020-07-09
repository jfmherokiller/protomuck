set(CSRC
        array.c
        boolexp.c
        cgi.c
        compile.c
        compress.c
        create.c db.c
        debugger.c
        disassem.c
        diskprop.c
        edit.c
        events.c
        game.c
        hashtab.c
        help.c
        inst.c
        interp.c
        log.c
        look.c
        match.c
        mcp.c
        mcppkgs.c
        mcpgui.c
        mfuns.c
        mfuns2.c
        mfuns3.c
        msgparse.c
        move.c
        mufevent.c
        crt_malloc.c
        oldcompress.c
        player.c
        predicates.c
        p_array.c
        p_connects.c
        p_db.c
        p_error.c
        p_float.c
        p_file.c
        p_html.c
        p_http.c
        p_math.c
        p_mcp.c
        p_misc.c
        p_muf.c
        p_mysql.c
        p_props.c
        p_socket.c
        p_stack.c
        p_strings.c
        p_system.c
        p_regex.c
        property.c
        propdirs.c
        props.c
        random.c
        rob.c
        set.c
        signal.c
        smatch.c
        speech.c
        strftime.c
        stringutil.c
        timequeue.c
        timestamp.c
        tune.c
        unparse.c
        utils.c
        wiz.c
        reg.c
        netresolve.c
        sanity.c
        newhttp.c
        version.c
        sha1.c
        other_interface_functions.c interface_module.c)
include_directories(inc)
find_package(OpenSSL REQUIRED)
find_package(MySQL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PCRE REQUIRED)
include_directories(${MYSQL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${PCRE_INCLUDE_DIRS})
set(MYLIBS ${ZLIB_LIBRARY} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} ${PCRE_LIBRARIES} ${OPENSSL_LIBRARIES} ${MYSQL_LIBRARIES})
if (UNIX)
    set(MYLIBS ${MYLIBS} "m" "nsl")
endif ()
if (WIN32)
    set(APPEND MYLIBS wsock32.lib)
endif ()
add_definitions(-DHAVE_CONFIG_H)
link_libraries(${MYLIBS})
add_executable(protoMuck ${CSRC} telopt.c mth.c msdp.c interface.c)
add_executable(resolver resolver.c)
add_executable(reslvd reslvd.c)
add_executable(mancheck mancheck.c)
#seems to be broken atm by the profiling functions
add_executable(topwords topwords.c hashtab.c crt_malloc.c)
#requires ARCHAIC_DATABASES
#add_executable(olddecompress olddecomp.c oldcompress.c)