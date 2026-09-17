#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "t3/orderbook/order_book.hpp"

namespace py = pybind11;
using namespace t3::orderbook;

PYBIND11_MODULE(_native, module) {
    module.doc() = "Native Track 3 order-book scaffold";

    py::enum_<Side>(module, "Side")
        .value("BID", Side::Bid)
        .value("ASK", Side::Ask);

    py::class_<Order>(module, "Order")
        .def(
            py::init([](OrderId id, AgentId agent_id, Side side, Price price,
                        Quantity quantity, TimeNs timestamp) {
                return Order{id, agent_id, side, price, quantity, timestamp};
            }),
            py::arg("id"), py::arg("agent_id"), py::arg("side"), py::arg("price"),
            py::arg("quantity"), py::arg("timestamp")
        )
        .def_readwrite("id", &Order::id)
        .def_readwrite("agent_id", &Order::agent_id)
        .def_readwrite("side", &Order::side)
        .def_readwrite("price", &Order::price)
        .def_readwrite("quantity", &Order::quantity)
        .def_readwrite("timestamp", &Order::timestamp);

    py::class_<BestQuote>(module, "BestQuote")
        .def_readonly("price", &BestQuote::price)
        .def_readonly("quantity", &BestQuote::quantity);

    py::class_<Fill>(module, "Fill")
        .def_readonly("resting_order_id", &Fill::resting_order_id)
        .def_readonly("incoming_order_id", &Fill::incoming_order_id)
        .def_readonly("resting_agent_id", &Fill::resting_agent_id)
        .def_readonly("incoming_agent_id", &Fill::incoming_agent_id)
        .def_readonly("price", &Fill::price)
        .def_readonly("quantity", &Fill::quantity);

    py::class_<MatchResult>(module, "MatchResult")
        .def_readonly("fills", &MatchResult::fills)
        .def_readonly("remaining_quantity", &MatchResult::remaining_quantity)
        .def_readonly("rested", &MatchResult::rested);

    py::enum_<CancelStatus>(module, "CancelStatus")
        .value("CANCELLED", CancelStatus::Cancelled)
        .value("NOT_FOUND", CancelStatus::NotFound);

    py::class_<CancelResult>(module, "CancelResult")
        .def_readonly("status", &CancelResult::status)
        .def_readonly("cancelled_quantity", &CancelResult::cancelled_quantity);

    py::class_<OrderBook>(module, "OrderBook")
        .def(py::init<>())
        .def("add_limit_order", &OrderBook::add_limit_order)
        .def("cancel", &OrderBook::cancel)
        .def("best_bid", &OrderBook::best_bid)
        .def("best_ask", &OrderBook::best_ask)
        .def("contains", &OrderBook::contains)
        .def_property_readonly("active_order_count", &OrderBook::active_order_count)
        .def("clear", &OrderBook::clear);
}

