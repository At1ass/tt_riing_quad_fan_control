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

template <typename T>
struct IsStdArrayOfUchar : std::false_type {};

template <std::size_t N>
struct IsStdArrayOfUchar<std::array<unsigned char, N>> : std::true_type {};

template <typename T>
inline constexpr bool IS_STD_ARRAY_OF_UCHAR_V = IsStdArrayOfUchar<T>::value;

template <typename T>
inline constexpr bool ALWAYS_FALSE_V = false;

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
            hid_open(vendorId, pid, empty ? NULL : serial_number.data()),
            deleter);

        if (dev.get() == NULL) {
            throw std::runtime_error(
                constructError("Failed hid_open: ", hid_error(NULL)));
        }

        core::Logger::log(core::LogLevel::INFO)
            << "Maked device with product_id " << pid << std::endl;

        return dev;
    }

    template <uint16_t vendorId>
    std::unique_ptr<hid_device, std::function<void(hid_device*)>> makeDevice(
        char const* path) {
        auto deleter = [](hid_device* dev) {
            if (dev != nullptr) {
                hid_close(dev);
            }
        };
        std::unique_ptr<hid_device, std::function<void(hid_device*)>> dev(
            hid_open_path(path), deleter);

        if (dev.get() == NULL) {
            throw std::runtime_error(
                constructError("Failed hid_open_path: ", hid_error(NULL)));
        }

        core::Logger::log(core::LogLevel::INFO)
            << "Maked device with path " << path << std::endl;

        return dev;
    }

    template <typename Container, typename U>
    void appendBytes(Container& buf, std::size_t& offset, U&& value) {
        using CleanU = std::decay_t<U>;

        if constexpr (std::is_integral_v<CleanU>) {
            buf[offset++] = static_cast<unsigned char>(value);
        } else if constexpr (std::is_enum_v<CleanU>) {
            buf[offset++] = static_cast<unsigned char>(value);
        }
        else if constexpr (IS_STD_ARRAY_OF_UCHAR_V<CleanU>) {
            for (auto b : value) {
                buf[offset++] = b;
            }
        } else {
            static_assert(ALWAYS_FALSE_V<U>,
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
        std::array<unsigned char, packet_size> usb_buf{0};
        std::size_t offset = 0;
        usb_buf[offset++] = start_byte;
        usb_buf[offset++] = f_prot;
        usb_buf[offset++] = s_prot;

        (appendBytes(usb_buf, offset, std::forward<T>(data)), ...);

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

        return response;
    }

    template <uint16_t vendorId, std::size_t N>
    std::generator<uint16_t> getHidEnumerationGeneratorPids(std::array<uint16_t, N> const PRODUCT_IDS) {
        auto devs = getHidEnumeration<vendorId>();
        hid_device_info* tmp = devs.get();

        while (tmp != nullptr) {
            if (std::find(PRODUCT_IDS.begin(), PRODUCT_IDS.end(), tmp->product_id) != PRODUCT_IDS.end()) {
                co_yield tmp->product_id;
            }
            tmp = tmp->next;
        }
    }

    template <uint16_t vendorId, std::size_t N>
    std::generator<char const*> getHidEnumerationGeneratorPaths(
        std::array<uint16_t, N> const PRODUCT_IDS) {
        auto devs = getHidEnumeration<vendorId>();
        hid_device_info* tmp = devs.get();

        while (tmp != nullptr) {
            if (std::find(PRODUCT_IDS.begin(), PRODUCT_IDS.end(), tmp->product_id) != PRODUCT_IDS.end()) {
                co_yield tmp->path;
            }
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
