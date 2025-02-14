#ifndef __HIDAPI_HPP__
#define __HIDAPI_HPP__

#include <stdint.h>

#include <codecvt>
#include <functional>
#include <generator>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "core/logger.hpp"
#include "hidapi.h"

constexpr std::size_t const MAX_STR = 256;

// -------------------------------------------------------------------
// 1) Служебные type traits для распознавания std::array<unsigned char, N>.
//
// Сначала - универсальный шаблон, по умолчанию false:
template <typename T>
struct is_std_array_of_uchar : std::false_type {};

// Специализация: если T = std::array<unsigned char, N>, то true
template <std::size_t N>
struct is_std_array_of_uchar<std::array<unsigned char, N>> : std::true_type {};

// Удобная переменная для if constexpr:
template <typename T>
inline constexpr bool is_std_array_of_uchar_v = is_std_array_of_uchar<T>::value;

// -------------------------------------------------------------------
// 2) "always_false_v" для статических_assert-ов в ветках if constexpr

template <typename T>
inline constexpr bool always_false_v = false;

namespace sys {

class HidApi {
   public:
    HidApi() {
        int ret = hid_init();

        if (ret != 0) {
            throw std::runtime_error("Cannot iniztialize hidapi");
        }
    }

    ~HidApi() { hid_exit(); }

    template <uint16_t vendorId>
    std::unique_ptr<hid_device, std::function<void(hid_device*)>> makeDevice(
        uint16_t pid, std::wstring_view serial_number = {}) {
        auto empty = serial_number.empty();
        auto deleter = [](hid_device* dev) {
            if (dev != nullptr) {
                hid_close(dev);
            }
        };
        std::unique_ptr<hid_device, std::function<void(hid_device*)>> dev(
            hid_open(vendorId, pid, empty ? nullptr : serial_number.data()),
            deleter);

        if (dev.get() == nullptr) {
            throw std::runtime_error(
                constructError("Failed hid_open: ", hid_error(NULL)));
        }

        core::Logger::log(core::LogLevel::INFO)
            << "Maked device with product_id " << pid << std::endl;

        return dev;
    }

    template <typename Container, typename U>
    void appendBytes(Container& buf, std::size_t& offset, U&& value) {
        using CleanU = std::decay_t<U>;

        // Если это целочисленный тип => трактуем как один байт
        if constexpr (std::is_integral_v<CleanU>) {
            buf[offset++] = static_cast<unsigned char>(value);
        } else if constexpr (std::is_enum_v<CleanU>) {
            // Enum (включая enum class)
            // Считаем, что хотим 1 байт
            buf[offset++] = static_cast<unsigned char>(value);
        }
        // Если это std::array<unsigned char, N>, копируем все элементы
        else if constexpr (is_std_array_of_uchar_v<CleanU>) {
            for (auto b : value) {
                buf[offset++] = b;
            }
        } else {
            static_assert(always_false_v<U>,
                          "Unsupported type in appendBytes (must be integral "
                          "or array<unsigned char, N>)");
        }
    }

    template <std::size_t packet_size, typename... T>
    void sendRequest(
        std::unique_ptr<hid_device, std::function<void(hid_device*)>>& dev,
        unsigned char start_byte, unsigned char f_prot, unsigned char s_prot,
        T... data) {
        static_assert(packet_size >= 3 + sizeof...(data),
                      "Too much data arguments");
        std::array<unsigned char, packet_size> usb_buf{};
        std::size_t offset = 0;
        // start_byte, f_prot, s_prot, static_cast<unsigned char>(data)...};
        usb_buf[offset++] = start_byte;
        usb_buf[offset++] = f_prot;
        usb_buf[offset++] = s_prot;

        // Раскладываем все остальные аргументы data... через fold-expression
        (appendBytes(usb_buf, offset, std::forward<T>(data)), ...);

        // Проверяем, чтобы не вылезли за packet_size
        if (offset > packet_size) {
            throw std::runtime_error(
                "Too many bytes for the given packet_size");
        }
        int ret = 0;

        ret = hid_write(dev.get(), usb_buf.data(), packet_size);
        if (ret == -1) {
            throw std::runtime_error(
                constructError("Failed hid_write: ", hid_error(dev.get())));
        }

        core::Logger::log(core::LogLevel::INFO) << "Send request" << std::endl;
    }

    template <std::size_t packet_size, std::size_t timeout = 0>
    std::array<unsigned char, packet_size> readResponse(
        std::unique_ptr<hid_device, std::function<void(hid_device*)>>& dev) {
        std::array<unsigned char, packet_size> response{0};
        int ret = 0;

        if (timeout == 0) {
            ret = hid_read(dev.get(), response.data(), packet_size);
        } else {
            ret = hid_read_timeout(dev.get(), response.data(), packet_size,
                                   timeout);
        }
        if (ret == -1) {
            throw std::runtime_error(constructError("Failed hid_read_timeout: ",
                                                    hid_error(dev.get())));
        }

        core::Logger::log(core::LogLevel::INFO) << "Read response" << std::endl;
        return response;
    }

    template <uint16_t vendorId>
    std::generator<uint16_t> getHidEnumerationGeneratorPids() {
        auto devs = getHidEnumeration<vendorId>();
        hid_device_info* tmp = devs.get();

        while (tmp != nullptr) {
            co_yield tmp->product_id;
            tmp = tmp->next;
        }
    }

    template <std::size_t NUM_CHANNELS>
    void printInfo(
        std::unique_ptr<hid_device, std::function<void(hid_device*)>>& dev) {
        std::array<wchar_t, MAX_STR> name_string{};
        int ret = 0;

        ret =
            hid_get_manufacturer_string(dev.get(), name_string.data(), MAX_STR);
        if (ret == -1) {
            throw std::runtime_error(constructError(
                "Failed hid_get_manufacturer_string: ", hid_error(dev.get())));
        }

        std::wprintf(L"Name: %s\n", name_string.data());  // NOLINT
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        core::Logger::log(core::LogLevel::INFO)
            << "Name: " << converter.to_bytes(name_string.data())
            << std::endl;  // NOLINT

        ret = hid_get_product_string(dev.get(), name_string.data(), MAX_STR);
        if (ret == -1) {
            throw std::runtime_error(constructError(
                "Failed hid_get_product_string: ", hid_error(dev.get())));
        }

        std::wprintf(L"Prod Name: %s\n", name_string.data());  // NOLINT
        core::Logger::log(core::LogLevel::INFO)
            << "Prod Name: " << converter.to_bytes(name_string.data())
            << std::endl;  // NOLINT
    }

   private:
    HidApi(HidApi const&) = default;
    HidApi(HidApi&&) = delete;
    HidApi& operator=(HidApi const&) = default;
    HidApi& operator=(HidApi&&) = delete;

    std::string constructError(std::string const& msg, wchar_t const* reason) {
        std::string err_msg("Failed hid_open: ");
        std::wstring error(reason);
        err_msg += std::string(error.begin(), error.end());
        return err_msg;
    }

    template <uint16_t vendorId, uint16_t productId = 0>
    auto getHidEnumeration() {
        auto deleter = [](hid_device_info* devs) {
            if (devs != nullptr) {
                hid_free_enumeration(devs);
            }
        };
        std::unique_ptr<hid_device_info, decltype(deleter)> devs(
            hid_enumerate(vendorId, productId), deleter);

        if (devs.get() == nullptr) {
            throw std::runtime_error(
                constructError("Failed hid_enumerate: ", hid_error(NULL)));
        }

        core::Logger::log(core::LogLevel::INFO)
            << "Get hid_enumerate" << std::endl;
        return devs;
    }
};

}  // namespace sys

#endif  // !__HIDAPI_HPP__
