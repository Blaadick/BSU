module;

#ifdef __linux__
#include <cerrno>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#elif _WIN32
#include <cstdlib>
#include <x86intrin.h>
#include <windows.h>
#include <shobjidl.h>
#endif

module bsu;

import std;

#ifdef __linux__
namespace {
    auto deviceOf(const std::filesystem::path& path) noexcept -> std::optional<dev_t> {
        struct stat pathStat{};
        if(lstat(path.c_str(), &pathStat) != 0) {
            return std::nullopt;
        }

        return std::make_optional(pathStat.st_dev);
    }

    auto mountPointOf(std::filesystem::path path, const dev_t device) -> std::filesystem::path {
        while(path.has_relative_path()) {
            auto parentPath = path.parent_path();
            if(deviceOf(parentPath) != device) {
                break;
            }

            path = std::move(parentPath);
        }

        return path;
    }

    auto isOwnedDir(const std::filesystem::path& path) noexcept -> bool {
        struct stat pathStat{};
        return lstat(path.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode) && pathStat.st_uid == getuid();
    }

    auto createTrashDir(const std::filesystem::path& trashDirPath) -> bool {
        for(const auto& dirPath : {trashDirPath, trashDirPath / "files", trashDirPath / "info"}) {
            if(mkdir(dirPath.c_str(), 0700) != 0 && errno != EEXIST) {
                return false;
            }

            if(!isOwnedDir(dirPath)) {
                errno = EPERM;
                return false;
            }
        }

        return true;
    }

    auto homeTrashDir() -> std::filesystem::path {
        const auto dataHomeEnv = std::getenv("XDG_DATA_HOME");
        const auto dataHomeDir = dataHomeEnv && *dataHomeEnv ? std::filesystem::path(dataHomeEnv) : std::filesystem::path(std::getenv("HOME")) / ".local" / "share";
        return dataHomeDir / "Trash";
    }

    auto topDirTrashDir(const std::filesystem::path& topDirPath) -> std::optional<std::filesystem::path> {
        const auto uid = std::to_string(getuid());
        const auto adminTrashDirPath = topDirPath / ".Trash";

        struct stat adminTrashStat{};
        if(
            lstat(adminTrashDirPath.c_str(), &adminTrashStat) == 0
            && S_ISDIR(adminTrashStat.st_mode)
            && adminTrashStat.st_mode & S_ISVTX
            && createTrashDir(adminTrashDirPath / uid)
        ) {
            return adminTrashDirPath / uid;
        }

        const auto userTrashDirPath = topDirPath / std::format(".Trash-{}", uid);
        if(createTrashDir(userTrashDirPath)) {
            return userTrashDirPath;
        }

        return std::nullopt;
    }

    auto percentEncode(const std::string_view str) -> std::string {
        std::string result;

        for(const unsigned char c : str) {
            if(
                (c >= 'A' && c <= 'Z')
                || (c >= 'a' && c <= 'z')
                || (c >= '0' && c <= '9')
                || c == '-' || c == '.' || c == '_' || c == '~'
            ) {
                result += static_cast<char>(c);
            } else {
                result += std::format("%{:02X}", c);
            }
        }

        return result;
    }

    auto encodeTrashInfoPath(const std::filesystem::path& path) -> std::string {
        std::string result = path.is_absolute() ? "/" : "";

        for(const auto& part : path.relative_path()) {
            if(!result.empty() && result.back() != '/') {
                result += '/';
            }

            result += percentEncode(part.string());
        }

        return result;
    }
}
#endif

auto bsu::filesystem::move_to_trash(const std::filesystem::path& path) -> bool {
    std::error_code errorCode;
    const auto result = move_to_trash(path, errorCode);
    if(errorCode) {
        throw std::filesystem::filesystem_error("cannot move to trash", path, errorCode);
    }

    return result;
}

auto bsu::filesystem::move_to_trash(const std::filesystem::path& path, std::error_code& errorCode) noexcept -> bool {
    const auto status = std::filesystem::symlink_status(path, errorCode);
    if(std::filesystem::exists(status)) {
        try {
            #ifdef __linux__
            const auto absolutePath = std::filesystem::absolute(path, errorCode).lexically_normal();
            if(errorCode) {
                return false;
            }

            const auto pathDevice = deviceOf(absolutePath);
            if(!pathDevice.has_value()) {
                errorCode.assign(errno, std::generic_category());
                return false;
            }

            const auto homeTrashDirPath = homeTrashDir();
            std::filesystem::create_directories(homeTrashDirPath.parent_path(), errorCode);

            std::filesystem::path trashDirPath;
            std::filesystem::path trashInfoPath;

            if(deviceOf(homeTrashDirPath.parent_path()) == pathDevice && createTrashDir(homeTrashDirPath)) {
                trashDirPath = homeTrashDirPath;
                trashInfoPath = absolutePath;
            } else {
                const auto topDirPath = mountPointOf(absolutePath, *pathDevice);
                const auto topDirTrashDirPath = topDirTrashDir(topDirPath);
                if(!topDirTrashDirPath.has_value()) {
                    errorCode.assign(errno, std::generic_category());
                    return false;
                }

                trashDirPath = *topDirTrashDirPath;
                trashInfoPath = absolutePath.lexically_relative(topDirPath);
            }

            const auto now = std::time(nullptr);
            std::tm localNow{};
            if(localtime_r(&now, &localNow) == nullptr) {
                errorCode.assign(errno, std::generic_category());
                return false;
            }

            std::array<char, 20> deletionDate{};
            std::strftime(deletionDate.data(), deletionDate.size(), "%FT%T", &localNow);

            const auto trashInfo = std::format(
                "[Trash Info]\nPath={}\nDeletionDate={}\n",
                encodeTrashInfoPath(trashInfoPath),
                deletionDate.data()
            );

            const auto baseName = absolutePath.filename().string();

            for(int i = 1;; ++i) {
                const auto trashName = i == 1 ? baseName : std::format("{}.{}", baseName, i);
                const auto trashedFilePath = trashDirPath / "files" / trashName;
                const auto infoFilePath = trashDirPath / "info" / (trashName + ".trashinfo");

                if(std::filesystem::exists(std::filesystem::symlink_status(trashedFilePath, errorCode))) {
                    continue;
                }

                const auto infoFile = open(infoFilePath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
                if(infoFile < 0) {
                    if(errno == EEXIST) {
                        continue;
                    }

                    errorCode.assign(errno, std::generic_category());
                    return false;
                }

                const auto written = write(infoFile, trashInfo.data(), trashInfo.size());
                if(written != static_cast<ssize_t>(trashInfo.size())) {
                    if(written < 0) {
                        errorCode.assign(errno, std::generic_category());
                    } else {
                        errorCode = std::make_error_code(std::errc::io_error);
                    }

                    close(infoFile);
                    unlink(infoFilePath.c_str());
                    return false;
                }

                if(close(infoFile) != 0 || std::rename(absolutePath.c_str(), trashedFilePath.c_str()) != 0) {
                    errorCode.assign(errno, std::generic_category());
                    unlink(infoFilePath.c_str());
                    return false;
                }

                errorCode.clear();
                return true;
            }
            #elif _WIN32
            const auto absolutePath = std::filesystem::absolute(path, errorCode).lexically_normal();
            if(errorCode) {
                return false;
            }

            const auto comInitResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

            IFileOperation* fileOperation = nullptr;
            IShellItem* item = nullptr;
            BOOL aborted = FALSE;

            auto result = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&fileOperation));
            if(SUCCEEDED(result)) {
                result = fileOperation->SetOperationFlags(FOF_ALLOWUNDO | FOFX_RECYCLEONDELETE | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT);
            }

            if(SUCCEEDED(result)) {
                result = SHCreateItemFromParsingName(absolutePath.c_str(), nullptr, IID_PPV_ARGS(&item));
            }

            if(SUCCEEDED(result)) {
                result = fileOperation->DeleteItem(item, nullptr);
            }

            if(SUCCEEDED(result)) {
                result = fileOperation->PerformOperations();
            }

            if(SUCCEEDED(result)) {
                result = fileOperation->GetAnyOperationsAborted(&aborted);
            }

            if(item) {
                item->Release();
            }

            if(fileOperation) {
                fileOperation->Release();
            }

            if(SUCCEEDED(comInitResult)) {
                CoUninitialize();
            }

            if(FAILED(result)) {
                errorCode.assign(static_cast<int>(result), std::system_category());
                return false;
            }

            if(aborted) {
                errorCode = std::make_error_code(std::errc::operation_canceled);
                return false;
            }

            errorCode.clear();
            return true;
            #endif
        } catch(const std::bad_alloc&) {
            errorCode = std::make_error_code(std::errc::not_enough_memory);
        }
    } else if(std::filesystem::status_known(status)) {
        errorCode.clear();
    }

    return false;
}
