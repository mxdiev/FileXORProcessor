# FileXORProcessor
Qt/C++ приложение для Windows: многопоточный инструмент для пакетной XOR-модификации файлов с поддержкой обработки больших объёмов данных (10GB+).

### Требования
- **ОС:** Windows 7/10/11
- **Qt:** 6.x или 5.15+
- **Компилятор:** MinGW-w64 или MSVC
- **CMake:** 3.16+

### Сборка из исходников

1. Клонируйте репозиторий:
```bash
git clone https://github.com/yourusername/FileXORProcessor.git
cd FileXORProcessor
2. Создайте папку для сборки:
```bash
mkdir build
cd build
3. Сгенерируйте проект с помощью CMake:
```bash
cmake .. -G "MinGW Makefiles"
# или для Qt Creator просто откройте CMakeLists.txt
4. Соберите проект:
```bash
cmake --build . --config Release
