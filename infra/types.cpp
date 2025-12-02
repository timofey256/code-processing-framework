#include "infra/types.hpp"

std::ostream& operator<<(std::ostream& os, request_type t) {
    switch (t) {
        case request_type::GET: return os << "GET";
        case request_type::POST: return os << "POST";
        case request_type::PUT: return os << "PUT";
        case request_type::DELETE_: return os << "DELETE_";
        case request_type::HEAD: return os << "HEAD";
        case request_type::OPTIONS: return os << "OPTIONS";
        case request_type::PATCH: return os << "PATCH";
    }
    return os << "UNKNOWN";
}
