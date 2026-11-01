# Blaad's System Utilities
A tiny cross-platform library that implements some basic system functions that, for some reason, not included in std library.

Some of the functionality was written by AI.

# Examples
## Filesystem
bsu::filesystem::move_to_trash():
```c++
import std;
import bsu;

int main(int argc, char* argv[]) {
    if(argc < 2) {
        return 1;
    }

    auto path = argv[1];
    bsu::filesystem::move_to_trash(path);
}
```

## Execute
bsu::execute():
```c++
import std;
import bsu;

int main(int argc, char* argv[]) {
    if(argc < 2) {
        return 1;
    }

    auto command = argv[1];
    auto env = std::array{std::string("GREETING=hello")};
    auto exitCode = bsu::execute(command, env);
    if(!exitCode) {
        std::println(std::cerr, "{}", exitCode.error());
        return 1;
    }

    return *exitCode;
}
```

## Secrets
bsu::secrets::get():
```c++
import std;
import bsu;

int main(int argc, char* argv[]) {
    if(argc < 2) {
        return 1;
    }

    auto service = "my-app";
    auto name = argv[1];
    auto secret = bsu::secrets::get(service, name);
    if(!secret) {
        std::println(std::cerr, "{}", secret.error());
        return 1;
    }

    std::println("{}", *secret);
}
```

bsu::secrets::set():
```c++
import std;
import bsu;

int main(int argc, char* argv[]) {
    if(argc < 3) {
        return 1;
    }

    auto service = "my-app";
    auto name = argv[1];
    auto value = argv[2];
    auto result = bsu::secrets::set(service, name, value);
    if(!result) {
        std::println(std::cerr, "{}", result.error());
        return 1;
    }
}
```
