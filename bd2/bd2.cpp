#include <vector>
#include <string>
#include <tuple>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>


// (Clases Sector, Pista, Superficie, Plato se mantienen iguales)
class Sector {
public:
    std::vector<char> datos;
    size_t capacidad;
    size_t espacioLibre;

    Sector(size_t size) : capacidad(size), espacioLibre(size) {}

    bool insertar_datos(const std::vector<char>& data, size_t& dataOffset) {
        size_t dataSize = data.size() - dataOffset;

        while (dataSize > 0 && espacioLibre > 0) {
            size_t fragmentSize = std::min(dataSize, espacioLibre);

            datos.insert(datos.end(), data.begin() + dataOffset, data.begin() + dataOffset + fragmentSize);
            espacioLibre -= fragmentSize;
            dataOffset += fragmentSize;
            dataSize -= fragmentSize;

            if (dataSize == 0) return true; // Dato completamente insertado
        }
        return false; // Aún queda dato por insertar
    }
};

// Clase Pista
class Pista {
public:
    int id;
    std::vector<Sector> sectores;

    Pista(int id, size_t numSectores, size_t capacidadSector) : id(id) {
        for (size_t i = 0; i < numSectores; ++i) {
            sectores.emplace_back(capacidadSector);
        }
    }
};

// Clase Superficie
class Superficie {
public:
    int id;
    std::vector<Pista> pistas;

    Superficie(int id, size_t numPistas, size_t numSectores, size_t capacidadSector) : id(id) {
        for (size_t i = 0; i < numPistas; ++i) {
            pistas.emplace_back(i, numSectores, capacidadSector);
        }
    }
};

// Clase Plato
class Plato {
public:
    int id;
    Superficie superior;
    Superficie inferior;

    Plato(int id, size_t numPistas, size_t numSectores, size_t capacidadSector)
        : id(id),
        superior(id * 2, numPistas, numSectores, capacidadSector),
        inferior(id * 2 + 1, numPistas, numSectores, capacidadSector) {
    }
};
// Clase Disco
class Disco {
public:
    std::vector<Plato> platos;
    std::map<std::string, std::vector<std::pair<std::string, std::tuple<int, int, int, int>>>> mapaTuplas;

    Disco(size_t numPlatos, size_t numPistas, size_t numSectores, size_t capacidadSector) {
        for (size_t i = 0; i < numPlatos; ++i) {
            platos.emplace_back(i, numPistas, numSectores, capacidadSector);
        }
    }
    // Función para obtener columnas como una cadena separada por comas
    std::string columnas_a_cadena(const std::vector<std::string>& columnas) const {
        std::ostringstream os;
        for (size_t i = 0; i < columnas.size(); ++i) {
            os << columnas[i];
            if (i < columnas.size() - 1) os << ", ";
        }
        return os.str();
    }

    bool insertar_tuplas(const std::string& nombreTabla, const std::vector<std::pair<std::string, std::string>>& tuplas) {
        size_t dataOffset = 0;
        std::vector<std::pair<std::string, std::tuple<int, int, int, int>>> indices;

        for (const auto& tupla : tuplas) {
            const std::string& clave = tupla.first;
            const std::string& datos = tupla.second;
            std::vector<char> data(datos.begin(), datos.end());
            dataOffset = 0;

            for (size_t platoIdx = 0; platoIdx < platos.size(); ++platoIdx) {
                for (int supIdx = 0; supIdx < 2; ++supIdx) {
                    Superficie& superficie = (supIdx == 0) ? platos[platoIdx].superior : platos[platoIdx].inferior;
                    for (auto& pista : superficie.pistas) {
                        for (size_t sectorIdx = 0; sectorIdx < pista.sectores.size(); ++sectorIdx) {
                            if (pista.sectores[sectorIdx].insertar_datos(data, dataOffset)) {
                                indices.emplace_back(clave, std::make_tuple(platoIdx, supIdx, pista.id, sectorIdx));
                                goto NEXT_TUPLA; // Salir del bucle interno
                            }
                        }
                    }
                }
            }
        NEXT_TUPLA:;
        }
        mapaTuplas[nombreTabla] = indices;
        return true;
    }

    void buscar_tupla(const std::string& nombreTabla, const std::string& clave) const {
        if (mapaTuplas.find(nombreTabla) != mapaTuplas.end()) {
            const auto& tuplas = mapaTuplas.at(nombreTabla);
            for (const auto& tupla : tuplas) {
                if (tupla.first == clave) {
                    auto idx = tupla.second;
                    int platoIdx = std::get<0>(idx);
                    int supIdx = std::get<1>(idx);
                    int pistaIdx = std::get<2>(idx);
                    int sectorIdx = std::get<3>(idx);
                    std::cout << "Clave: " << clave << " encontrada en -> "
                        << "Plato: " << platoIdx << ", Superficie: " << supIdx
                        << ", Pista: " << pistaIdx << ", Sector: " << sectorIdx << "\n";
                    return;
                }
            }
            std::cout << "Clave: " << clave << " no encontrada en la tabla \"" << nombreTabla << "\".\n";
        }
        else {
            std::cout << "La tabla \"" << nombreTabla << "\" no existe en el disco.\n";
        }
    }
};

// Función para procesar el archivo SQL
std::vector<std::string> procesar_archivo_sql(const std::string& ruta) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo SQL: " + ruta);
    }

    std::vector<std::string> columnas;
    std::string linea;

    // Leer las columnas definidas en el CREATE TABLE
    while (std::getline(archivo, linea)) {
        std::transform(linea.begin(), linea.end(), linea.begin(), ::tolower);
        if (linea.find("create table") != std::string::npos || linea.find(");") != std::string::npos) {
            continue; // Ignorar líneas que no definen columnas
        }
        size_t pos = linea.find(' ');
        if (pos != std::string::npos) {
            columnas.push_back(linea.substr(0, pos)); // Extraer el nombre de la columna
        }
    }
    return columnas;
}
// Función para procesar archivo CSV
std::vector<std::pair<std::string, std::string>> procesar_archivo_csv(const std::string& ruta) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo CSV: " + ruta);
    }

    std::vector<std::pair<std::string, std::string>> tuplas;
    std::string linea;

    while (std::getline(archivo, linea)) {
        std::istringstream iss(linea);
        std::string clave, resto;

        // Leer la clave (primera columna)
        std::getline(iss, clave, ',');
        clave.erase(std::remove(clave.begin(), clave.end(), ' '), clave.end()); // Eliminar espacios

        // Leer el resto de la fila como datos
        std::getline(iss, resto);
        resto.erase(std::remove(resto.begin(), resto.end(), '"'), resto.end()); // Eliminar comillas
        resto.erase(0, resto.find_first_not_of(' ')); // Quitar espacios al inicio

        tuplas.emplace_back(clave, resto);
    }
    return tuplas;
}
void SetupImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

// Manejador para la búsqueda
void buscar_y_mostrar(const Disco& disco, const std::string& tabla, const std::string& criterio, const std::string& valor, std::string& resultado) {
    std::ostringstream os;
    if (criterio == "Clave") {
        if (disco.mapaTuplas.find(tabla) != disco.mapaTuplas.end()) {
            const auto& tuplas = disco.mapaTuplas.at(tabla);
            for (const auto& tupla : tuplas) {
                if (tupla.first == valor) {
                    auto idx = tupla.second;
                    os << "Clave: " << valor << " encontrada en -> "
                        << "Plato: " << std::get<0>(idx) << ", Superficie: " << std::get<1>(idx)
                        << ", Pista: " << std::get<2>(idx) << ", Sector: " << std::get<3>(idx);
                    resultado = os.str();
                    return;
                }
            }
        }
        resultado = "Clave no encontrada.";
    }
    else if (criterio == "Índice") {
        try {
            int index = std::stoi(valor);
            if (disco.mapaTuplas.find(tabla) != disco.mapaTuplas.end()) {
                const auto& tuplas = disco.mapaTuplas.at(tabla);
                if (index >= 0 && index < tuplas.size()) {
                    const auto& tupla = tuplas[index];
                    auto idx = tupla.second;
                    os << "Índice: " << index << " encontrado en -> "
                        << "Plato: " << std::get<0>(idx) << ", Superficie: " << std::get<1>(idx)
                        << ", Pista: " << std::get<2>(idx) << ", Sector: " << std::get<3>(idx);
                    resultado = os.str();
                    return;
                }
            }
            resultado = "Índice fuera de rango.";
        }
        catch (...) {
            resultado = "Índice inválido.";
        }
    }
    else {
        resultado = "Criterio no soportado.";
    }
}

int main() {
    // Configuración de GLFW
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(1200, 800, "Disco DB GUI", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Inicializar ImGui
    SetupImGui(window);

    // Configuración del disco
    size_t numPlatos = 2, numPistas = 4, numSectores = 8, capacidadSector = 64;
    Disco disco(numPlatos, numPistas, numSectores, capacidadSector);

    // Procesar archivos (ruta ajustable)
    std::string ruta_sql = "C:/Users/renat/Downloads/struct_table.txt";
    auto columnas = procesar_archivo_sql(ruta_sql);
    std::string ruta_csv = "C:/Users/renat/Downloads/taxables.csv";
    auto tuplas_csv = procesar_archivo_csv(ruta_csv);
    disco.insertar_tuplas("PRODUCTO", tuplas_csv);

    // Variables para la GUI
    char clave[64] = "";
    char datos[256] = "";
    char valorBusqueda[64] = "";
    std::string resultadoBusqueda = "Ingrese criterio y valor para buscar.";
    std::string resultadoEstado = "Estado del sistema.";

    // Opciones para el Combo
    const char* opcionesCriterio[] = { "Clave", "Índice" };
    int indiceCriterio = 0; // Índice seleccionado inicialmente

    // Loop de renderizado
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Ventana principal
        ImGui::Begin("Administración de Disco DB");

        // Panel de búsqueda
        ImGui::Text("Búsqueda de datos:");
        if (ImGui::Combo("Criterio", &indiceCriterio, opcionesCriterio, IM_ARRAYSIZE(opcionesCriterio))) {
            // El índice se actualiza automáticamente
        }
        ImGui::InputText("Valor", valorBusqueda, sizeof(valorBusqueda));
        if (ImGui::Button("Buscar")) {
            buscar_y_mostrar(disco, "PRODUCTO", opcionesCriterio[indiceCriterio], valorBusqueda, resultadoBusqueda);
        }

        ImGui::Separator();
        ImGui::TextWrapped("Resultado de búsqueda:");
        ImGui::TextWrapped("%s", resultadoBusqueda.c_str());

        ImGui::Separator();

        // Formulario para agregar nueva tupla
        ImGui::Text("Agregar nueva tupla:");
        ImGui::InputText("Clave", clave, sizeof(clave));
        ImGui::InputTextMultiline("Datos", datos, sizeof(datos));
        if (ImGui::Button("Agregar")) {
            std::vector<std::pair<std::string, std::string>> nuevaTupla = { { clave, datos } };
            disco.insertar_tuplas("PRODUCTO", nuevaTupla);
            resultadoEstado = "Tupla agregada con éxito.";
        }

        ImGui::End();

        // Panel lateral para mostrar estados
        ImGui::Begin("Estado del Sistema");
        ImGui::TextWrapped("%s", resultadoEstado.c_str());
        ImGui::End();

        // Renderizado
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Limpiar recursos de ImGui y GLFW
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}