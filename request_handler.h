#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"

// Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
// с другими подсистемами приложения.

namespace transport {

    class RequestHandler {
    public:
        RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer)
         : db_(db), renderer_(renderer) {
        }

        // Возвращает информацию об остановке (запрос Stop)
        std::optional<Stop> GetStopStat(const std::string_view& stop_name) const {
            return *db_.GetStop(stop_name);
        }
        
        // Возвращает информацию о маршруте (запрос Bus)
        std::optional<Bus> GetBusStat(const std::string_view& bus_name) const {
            return *db_.GetRoute(bus_name);
        }

        // Возвращает маршруты, проходящие через
        //const std::unordered_set<BusPtr>* GetBusesByStop(const std::string_view& stop_name) const;
        
        svg::Document RenderMap() const {
            return renderer_.Render();
        }

    private:
        // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
        const TransportCatalogue& db_;
        const renderer::MapRenderer& renderer_;
    };

}