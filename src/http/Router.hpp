#pragma once
#include <functional>
#include <regex>
#include <string>
#include <vector>

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace formaia::http {

namespace beast_http = boost::beast::http;

struct HttpContext {
    beast_http::request<beast_http::string_body>& req;
    std::vector<std::string> params; // capturas de la ruta, en orden ({id} -> params[0])
    nlohmann::json body;             // ya parseado si content-type es JSON y el body no está vacío
};

using HttpHandler = std::function<void(HttpContext&, beast_http::response<beast_http::string_body>&)>;

// Router muy simple pensado para un backend pequeño: cada ruta se declara
// como método + patrón (ej. "/api/solicitudes-ia/{id}"), que internamente
// se compila a una regex. No pretende reemplazar un framework completo,
// solo resolver el enrutamiento sin dependencias extra.
class Router {
public:
    void add(beast_http::verb metodo, const std::string& patron, HttpHandler handler) {
        rutas_.push_back({metodo, compilar(patron), handler});
    }

    bool despachar(beast_http::request<beast_http::string_body>& req,
                    beast_http::response<beast_http::string_body>& res) {
        std::string target = std::string(req.target());
        auto q = target.find('?');
        if (q != std::string::npos) target = target.substr(0, q);

        for (auto& ruta : rutas_) {
            if (ruta.metodo != req.method()) continue;
            std::smatch match;
            if (std::regex_match(target, match, ruta.regex)) {
                HttpContext ctx{req, {}, {}};
                for (size_t i = 1; i < match.size(); ++i) ctx.params.push_back(match[i].str());

                auto ct = req[beast_http::field::content_type];
                if (!req.body().empty() && ct.find("application/json") != std::string_view::npos) {
                    try { ctx.body = nlohmann::json::parse(req.body()); }
                    catch (...) {
                        res.result(beast_http::status::bad_request);
                        res.set(beast_http::field::content_type, "application/json");
                        res.body() = R"({"error":"JSON invalido en el cuerpo de la peticion"})";
                        res.prepare_payload();
                        return true;
                    }
                }

                ruta.handler(ctx, res);
                return true;
            }
        }
        return false;
    }

private:
    struct Ruta {
        beast_http::verb metodo;
        std::regex regex;
        HttpHandler handler;
    };

    static std::regex compilar(const std::string& patron) {
        // Convierte "/api/rutinas/{id}" en la regex "^/api/rutinas/([^/]+)$"
        std::string regexStr = "^";
        size_t i = 0;
        while (i < patron.size()) {
            if (patron[i] == '{') {
                size_t fin = patron.find('}', i);
                regexStr += "([^/]+)";
                i = fin + 1;
            } else {
                if (std::string("\\^$.|?*+()[]{}").find(patron[i]) != std::string::npos)
                    regexStr += '\\';
                regexStr += patron[i];
                ++i;
            }
        }
        regexStr += "$";
        return std::regex(regexStr);
    }

    std::vector<Ruta> rutas_;
};

} // namespace formaia::http
