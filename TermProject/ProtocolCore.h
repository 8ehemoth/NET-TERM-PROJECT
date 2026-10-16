#pragma once
#include "ChatMsg.h"
#include "PrjClientFile.h"
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <limits>

constexpr int PROTOCOL_ID_MAX = 50;
constexpr int PROTOCOL_MSG_MAX = 512;
constexpr int FILE_NAME_MAX = 255;
constexpr int FILE_SIZE_MAX = 16 * 1024 * 1024;
constexpr int CALC_EXPR_MAX = 255;
constexpr int SERVER_TEXT_MAX = 2048;
static_assert(sizeof(int) == 4, "Protocol requires 32-bit int");
static_assert(sizeof(ChatMsg) == 12 && sizeof(FileMsg) == 16, "Unexpected protocol layout");

inline bool ValidId(const std::string& id) {
    if (id.empty() || id.size() > PROTOCOL_ID_MAX) return false;
    bool visible = false;
    for (unsigned char c : id) {
        if (c < 32 || c == 127) return false;
        if (c != ' ') visible = true;
    }
    return visible;
}
inline bool SafeFileName(const std::string& name) {
    if (name.empty() || name.size() > FILE_NAME_MAX || name == "." || name == "..") return false;
    if (name.back() == '.' || name.back() == ' ') return false;
    for (unsigned char c : name)
        if (c < 32 || c == 127 || std::strchr("<>:\"/\\|?*", c)) return false;
    std::string base = name.substr(0, name.find('.'));
    for (char& c : base) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if (base == "CON" || base == "PRN" || base == "AUX" || base == "NUL" || base == "CONIN$" || base == "CONOUT$") return false;
    if (base.size() == 4 && (base.substr(0,3) == "COM" || base.substr(0,3) == "LPT") && base[3] >= '1' && base[3] <= '9') return false;
    return true;
}
inline std::vector<char> ChatPacket(int type, const std::string& id, const std::string& text) {
    ChatMsg h{type, static_cast<int>(id.size()), static_cast<int>(text.size())};
    std::vector<char> out(sizeof(h) + id.size() + text.size());
    std::memcpy(out.data(), &h, sizeof(h));
    if (!id.empty()) std::memcpy(out.data()+sizeof(h), id.data(), id.size());
    if (!text.empty()) std::memcpy(out.data()+sizeof(h)+id.size(), text.data(), text.size());
    return out;
}
// Returns 0 for an incomplete frame, -1 for an invalid header, 1 for a known frame size.
inline int ClientFrameSize(const std::vector<char>& in, size_t& size) {
    if (in.size() < sizeof(int)) return 0;
    int type; std::memcpy(&type, in.data(), sizeof(type));
    if (type == MSG_ENROLL) {
        if (in.size() < sizeof(EnrollMsg)) return 0;
        EnrollMsg h; std::memcpy(&h,in.data(),sizeof(h));
        if (h.idLen <= 0 || h.idLen > PROTOCOL_ID_MAX) return -1;
        size=sizeof(h)+static_cast<size_t>(h.idLen);
    } else if (type == MSG_CHAT) {
        if (in.size() < sizeof(ChatMsg)) return 0;
        ChatMsg h; std::memcpy(&h,in.data(),sizeof(h));
        if (h.idLen <= 0 || h.idLen > PROTOCOL_ID_MAX || h.msgLen < 0 || h.msgLen > PROTOCOL_MSG_MAX) return -1;
        size=sizeof(h)+static_cast<size_t>(h.idLen)+static_cast<size_t>(h.msgLen);
    } else if (type == MSG_FILE) {
        if (in.size() < sizeof(FileMsg)) return 0;
        FileMsg h; std::memcpy(&h,in.data(),sizeof(h));
        if (h.nameLen <= 0 || h.nameLen > FILE_NAME_MAX || h.fileLen < 0 || h.fileLen > FILE_SIZE_MAX || (h.isText != 0 && h.isText != 1)) return -1;
        size=sizeof(h)+static_cast<size_t>(h.nameLen)+static_cast<size_t>(h.fileLen);
    } else return -1;
    return 1;
}
inline std::string EvaluateExpression(const std::string& expr) {
    if (expr.empty() || expr.size() > CALC_EXPR_MAX || expr.find('\0') != std::string::npos) return "ERROR: invalid expression";
    std::istringstream in(expr);
    double a=0,b=0; char op=0;
    if (!(in >> a >> op >> b)) return "ERROR: invalid expression";
    in >> std::ws;
    if (!in.eof() || !std::isfinite(a) || !std::isfinite(b)) return "ERROR: invalid expression";
    double result=0;
    switch(op) {
    case '+': result=a+b; break;
    case '-': result=a-b; break;
    case '*': result=a*b; break;
    case '/': if (b==0) return "ERROR: division by zero"; result=a/b; break;
    default: return "ERROR: invalid operator";
    }
    if (!std::isfinite(result)) return "ERROR: result out of range";
    std::ostringstream out; out << std::setprecision(12) << result; return out.str();
}
