# 🤖 Ada — Virtual Assistant

![Ada_Logo](Ada-ICON.png)

**Ada** es una asistente virtual de escritorio desarrollada nativamente en **C++** utilizando **SDL2** para la interfaz gráfica y la API de **OpenRouter** para el motor de inteligencia artificial. Cuenta con un sistema de voz fluido en español nativo a través de la API SAPI de Windows(se usa PicoTTS en la versión para Linux) y memoria persistente de conversación.

---

## 🚀 Características Principales

* **Motor de IA Avanzado:** Conexión de baja latencia con modelos a través de OpenRouter.
* **Memoria Persistente:** Historial de conversación guardado localmente en un buffer cíclico (`memories.json`).
* **Texto a Voz (TTS):** Integración directa con Windows SAPI configurado específicamente con la voz nativa de **Microsoft Sabina**(En la versión de Linux se usa PicoTTS).
* **Voz a Texto (STT):** La app usa **Vosk** para poder transcribir tu voz a texto, esto con el fin de poder dar prompts por voz y solo dar a Enter para enviar. Para usar esta característica presiona Shift + F1 en la app.
* **Capacidad de controlar tu PC:** Ella puede tanto apagar tu PC, reiniciarla, abrir apps de tu equipo, abrir sitios webs, ejecutar comandos del sistema y hacerte recordatorios para hoy o mañana.
* **Gestos**: Ada cambia de gesto dependiendo del tema que hables con ella. Esto es con el objetivo de que la comunicación con ella sea más interactiva
* **Modo Offline**: La app tiene un modo offline impulsado por [Llama](https://github.com/ggml-org/llama.cpp/tree/6eddde06a4f25d55d538b5d15628dcc2b6882147). Para poder usar este modo necesitas tener como mínimo 8GB de RAM. Se recomienda tener 16GB de RAM si desea usar el modo offline con una buena cantidad de apps abiertas en el equipo.

---

## Requisito inicial para utilizar a la asistente 

Para poder utilizarla necesitas una API Key de [OpenRouter](https://www.openrouter.ai). Para obtenerla haz los siguentes pasos:

* Obtener API Key:
    * Registra/Inicia sesión en [OpenRouter](https://www.openrouter.ai)

    * Crea una API Key presionando "New Key" luego de iniciar sesión/registrarse

    ![OpenRouterStep1](OpenRouterStep.png)

* Configuración inicial en la app

    * Guarda la API Key en un sitio seguro

    * Ingresa la API Key al momento de iniciar la app(por primera vez)

    * Diviertete con Ada ;)

---

## 🛠️ Requisitos de Desarrollo

Para compilar y correr este proyecto desde cero, necesitas las siguientes herramientas y dependencias:

### Dependencias en Linux (Ubuntu/Debian)
* **Compilador:** `g++` con soporte para C++17 o superior.
* **Librerías de Desarrollo:**
```bash
sudo apt install build-essential libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libttspico-dev libnotify-dev libcurl4-openssl-dev
```
* **Parser JSON: `nlohmann-json que ya está en ./include`**

### Dependencias para Windows(MinGW/MSYS2)
* **Compilador:** `g++` con soporte para C++17 o superior(x86_64).
* **Librerías gráficas: SDL2, SDL2_image, SDL2_ttf**
* **Librería de red: `libcurl`**

Para iniciar los submodulos(en este caso md4c y llama.cpp), ejecuta en la terminal:
 ```bash
 git submodule update --init --recursive
 ```
---

Si instalaste Ada para distros basadas en Debian(por el paquete .deb), se debe ejecutar desde la terminal con el comando:
```bash
ada
```

# Instrucciones para compilar
Si desea compilar Ada para tu OS anfitrión solo usa el comando `make`. En el caso de querer compilar para Windows desde Linux use el comando `make CROSS=1`. Para compilar en Windows solo se requiere el comando `make`.

Si quiere hacerle debugging a la app, compile la app con el parámetro DEBUG=1, ej: `make DEBUG=1`, `make DEBUG=1 CROSS=1 all`

Para limpiar la compilación de la app utilice el comando `make clean_app`. Luego para limpiar todo incluyendo la compilación de las dependencias(md4c y llama.cpp) use el comando `make clean_all`.

## Instrucciones para empaquetar
Con el comando `make make_debian_package` se crea el .deb de Ada, con `make install_debian_package` se instala el paquete en su equipo. En Linux al usar `make` automáticamente se crea el .tar.gz.

En Windows con el comando `make pack_for_windows` se crea el .zip para Windows

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
