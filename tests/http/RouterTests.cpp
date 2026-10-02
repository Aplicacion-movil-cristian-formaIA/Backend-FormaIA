#include <catch2/catch_test_macros.hpp>
#include "http/Router.hpp"
#include <boost/beast/http.hpp>

using namespace formaia::http;
namespace bhttp = boost::beast::http;

TEST_CASE("Router enruta correctamente", "[router]") {
    Router router;
    bool handlerLlamado = false;
    std::string paramExtraido = "";

    router.add(bhttp::verb::get, "/api/recurso/{id}", [&](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
        handlerLlamado = true;
        if (!ctx.params.empty()) {
            paramExtraido = ctx.params[0];
        }
        res.result(bhttp::status::ok);
        res.body() = "ok";
    });

    SECTION("Coincidencia exacta con parametros") {
        bhttp::request<bhttp::string_body> req;
        req.method(bhttp::verb::get);
        req.target("/api/recurso/123");

        bhttp::response<bhttp::string_body> res;
        bool despachado = router.despachar(req, res);

        REQUIRE(despachado == true);
        REQUIRE(handlerLlamado == true);
        REQUIRE(paramExtraido == "123");
        REQUIRE(res.result() == bhttp::status::ok);
    }

    SECTION("No coincide si el metodo es distinto") {
        bhttp::request<bhttp::string_body> req;
        req.method(bhttp::verb::post);
        req.target("/api/recurso/123");

        bhttp::response<bhttp::string_body> res;
        bool despachado = router.despachar(req, res);

        REQUIRE(despachado == false);
        REQUIRE(handlerLlamado == false);
    }

    SECTION("No coincide si la ruta es distinta") {
        bhttp::request<bhttp::string_body> req;
        req.method(bhttp::verb::get);
        req.target("/api/otro/123");

        bhttp::response<bhttp::string_body> res;
        bool despachado = router.despachar(req, res);

        REQUIRE(despachado == false);
        REQUIRE(handlerLlamado == false);
    }
}
