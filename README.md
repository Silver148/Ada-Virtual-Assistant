# 🤖 Ada — Virtual Assistant

**Ada** es una asistente virtual de escritorio desarrollada nativamente en **C++** utilizando **SDL2** para la interfaz gráfica y la API de **OpenRouter** para el motor de inteligencia artificial. Cuenta con un sistema de voz fluido en español nativo a través de la API SAPI de Windows y memoria persistente de conversación.

---

## 🚀 Características Principales

* **Motor de IA Avanzado:** Conexión de baja latencia con modelos a través de OpenRouter.
* **Memoria Persistente:** Historial de conversación guardado localmente en un buffer cíclico (`memories.json`).
* **Texto a Voz (TTS):** Integración directa con Windows SAPI configurado específicamente con la voz nativa de **Microsoft Sabina**.
* **Capacidad de controlar tu PC:** Ella puede tanto apagar tu PC, reiniciarla, abrir apps de tu equipo y hacerte recordatorios para hoy o mañana.

---

## 🛠️ Requisitos de Desarrollo

Para compilar y correr este proyecto desde cero necesitas las siguientes herramientas configuradas en tu entorno (ej. MinGW / MSYS2 en Windows):

* **Compilador:** `g++` con soporte para C++11 o superior.
* **Librerías Gráficas:** SDL2 (junto a sus módulos de desarrollo para MinGW x86_64).
* **Librería de Red:** `libcurl` para la gestión de peticiones HTTP POST.
* **Parser JSON:** `nlohmann/json` (incluido localmente en el header `json.hpp`).
* **Clonar submódulo md4c:** Para ello ejecuta el comando `git submodule update --init --recursive`