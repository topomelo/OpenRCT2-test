--- PROJECT_AUDIT_REPORT.md (原始)


+++ PROJECT_AUDIT_REPORT.md (修改后)
# Informe de Auditoría del Proyecto OpenRCT2

## Resumen Ejecutivo

**OpenRCT2** es una re-implementación open-source del videojuego RollerCoaster Tycoon 2 (RCT2), un juego de simulación de gestión de parques de atracciones. El proyecto está escrito principalmente en C++ moderno (C++20) y utiliza CMake como sistema de construcción.

**Licencia**: GNU General Public License v3.0+
**Líneas de código**: ~221,777 líneas (solo archivos .cpp y .h en src/openrct2)

---

## 1. Estructura del Proyecto

### 1.1 Directorios Principales

```
/workspace/
├── CMakeLists.txt              # Configuración principal de compilación
├── src/                        # Código fuente principal
│   ├── openrct2/               # Núcleo del juego (lógica, mundo, entidades)
│   ├── openrct2-ui/            # Interfaz de usuario y renderizado
│   ├── openrct2-cli/           # Herramientas de línea de comandos
│   ├── openrct2-data/          # Gestión de datos
│   ├── openrct2-deps/          # Dependencias
│   ├── openrct2-win/           # Punto de entrada Windows
│   ├── openrct2-android/       # Plataforma Android
│   └── thirdparty/             # Librerías de terceros
├── data/                       # Datos del juego (idiomas, shaders)
├── docs/                       # Documentación técnica
├── scripts/                    # Scripts de construcción y utilidades
├── test/                       # Pruebas unitarias
└── distribution/               # Archivos de distribución
```

### 1.2 Módulos del Núcleo (src/openrct2/)

#### **Módulos Principales:**

| Módulo | Descripción |
|--------|-------------|
| `actions/` | Sistema de acciones/comandos del juego (place, remove, set) |
| `audio/` | Sistema de audio y música |
| `config/` | Gestión de configuración y archivos INI |
| `core/` | Utilidades base (archivos, strings, memoria, HTTP, etc.) |
| `drawing/` | Motor de renderizado 2D y sprites |
| `entity/` | Entidades del juego (peeps, staff, globos, patos) |
| `interface/` | Sistema de ventanas y UI base |
| `localisation/` | Sistema de internacionalización (i18n) |
| `management/` | Gestión del parque (finanzas, investigación, noticias) |
| `network/` | Multijugador y red |
| `object/` | Sistema de objetos (rides, scenery, paths) |
| `paint/` | Sistema de pintado isométrico |
| `park/` | Lógica específica del parque |
| `peep/` | IA y comportamiento de visitantes/staff |
| `ride/` | Sistema de atracciones y vehículos |
| `scenario/` | Gestión de escenarios y objetivos |
| `scripting/` | API de scripting (JavaScript/QuickJS) |
| `world/` | Mapa, tiles, terreno y clima |
| `rct1/`, `rct2/`, `rct12/` | Compatibilidad con formatos originales |
| `scenes/` | Escenas del juego (título, editor, menú) |

---

## 2. Arquitectura del Sistema

### 2.1 Componentes Clave

#### **Contexto Principal (`Context.cpp`)**
- Punto central de coordinación del juego
- Gestiona servicios: localización, objetos, audio, UI, red, scripting
- Controla el bucle principal del juego y timing
- Maneja estados del juego y transiciones entre escenas

#### **GameState (`GameState.h/.cpp`)**
- Contiene todo el estado mutable del juego
- Serializable para guardado/carga y multijugador
- Incluye: mapa, entidades, parque, finanzas, investigación

#### **Sistema de Acciones (`actions/`)**
- Patrón Command para todas las operaciones del juego
- Soporte para deshacer/rehacer en modo single-player
- Sincronización determinista en multijugador
- Categorías: cheats, footpath, general, network, park, peep, ride, scenery, terraform, track

#### **Sistema de Objetos (`object/`)**
- Tipos: Audio, Banner, Climate, Entrance, Footpath, LargeScenery, Music, Ride, SceneryGroup, SmallScenery, Surface, Wall, Water
- Carga desde archivos .DAT originales o .json/.zip personalizados
- Repositorio e indexación de objetos disponibles

### 2.2 Flujo Principal del Juego

```
1. Inicialización (Context::Initialise)
   ├─ Cargar configuración
   ├─ Inicializar audio, UI, red
   ├─ Cargar objetos y escenarios
   └─ Mostrar escena de precarga

2. Bucle Principal (Context::Run)
   ├─ Procesar input (UI)
   ├─ Actualizar lógica (Game::Update)
   │   ├─ Ejecutar acciones pendientes
   │   ├─ Actualizar entidades (peeps, vehículos)
   │   ├─ Actualizar parque (finanzas, investigación)
   │   └─ Actualizar mundo (clima, animaciones)
   ├─ Renderizar (Drawing Engine)
   └─ Controlar FPS/timing

3. Escenas (SceneManager)
   ├─ PreloaderScene: Carga inicial
   ├─ TitleScene: Pantalla de título
   ├─ MainMenuScene: Menú principal
   ├─ GameScene: Juego activo
   └─ EditorScene: Editor de escenarios
```

---

## 3. Sistema de Mundo y Mapa

### 3.1 Estructura del Mapa

- **Tile Elements**: Cada tile puede tener múltiples elementos apilados
  - `SurfaceElement`: Terreno, agua, superficie
  - `PathElement`: Caminos y senderos
  - `TrackElement`: Vías de atracciones
  - `SmallSceneryElement`: Decoración pequeña
  - `LargeSceneryElement`: Decoración grande
  - `WallElement`: Muros/vallas
  - `BannerElement`: Carteles
  - `EntranceElement`: Entradas de atracciones

### 3.2 Coordenadas y Ubicación

- Mapa: 256x256 tiles máximo (expandible desde el original)
- Sistema de coordenadas 3D (x, y, z)
- Tile size: 32x32 unidades lógicas
- Altura: Múltiplos de 16 unidades

---

## 4. Sistema de Atracciones (Ride)

### 4.1 Componentes

| Archivo | Responsabilidad |
|---------|----------------|
| `Ride.cpp/h` | Lógica principal de atracción |
| `Vehicle.cpp/h` | Física y movimiento de vehículos |
| `Track.cpp/h` | Definición y construcción de vías |
| `TrackData.cpp/h` | Datos de piezas de vía |
| `RideRatings.cpp/h` | Cálculo de estadísticas (emoción, intensidad, náusea) |
| `RideConstruction.cpp/h` | Interfaz de construcción |
| `TrainManager.cpp/h` | Gestión de trenes/vehículos |

### 4.2 Tipos de Atracciones

- Montañas rusas (coasters)
- Atracciones suaves (gentle rides)
- Atracciones de emoción (thrill rides)
- Tiendas y puestos (shops)
- Transporte (transport)
- Acuáticas (water rides)

---

## 5. Entidades y Peeps

### 5.1 Sistema de Entidades

```
EntityBase (base)
├── Peep
│   ├── Guest (visitantes)
│   └── Staff (personal)
├── Vehicle (vehículos de atracciones)
├── Balloon (globos)
├── Duck (patos)
├── MoneyEffect (efectos de dinero)
├── Litter (basura)
├── JumpingFountain (fuentes)
└── Particle (partículas)
```

### 5.2 IA de Peeps

- Máquina de estados para comportamiento
- Sistema de pensamientos y decisiones
- Necesidades: hambre, sed, energía, higiene, diversión
- Navegación por pathfinding A*
- Preferencias personales por tipo de atracción

---

## 6. Multijugador y Red

### 6.1 Arquitectura de Red

- Modelo cliente-servidor
- Sincronización determinista de acciones
- Sistema de grupos y permisos
- Chat integrado
- Lista de servidores

### 6.2 Componentes de Red

| Clase | Función |
|-------|---------|
| `NetworkServer` | Servidor del juego |
| `NetworkClient` | Cliente conectado |
| `NetworkConnection` | Conexión individual |
| `NetworkPacket` | Paquetes de red |
| `NetworkPlayer` | Jugador conectado |
| `NetworkGroup` | Grupos de permisos |
| `NetworkAction` | Acciones con permisos |

---

## 7. Scripting y Plugins

### 7.1 Motor de Scripting

- Basado en QuickJS (JavaScript ES6+)
- API completa accesible desde scripts
- Hooks para eventos del juego
- Creación de ventanas y widgets personalizados

### 7.2 Bindings Disponibles

```
scripting/bindings/
├── entity/      # Manipulación de entidades
├── game/        # Funciones generales del juego
├── network/     # Funciones de red
├── object/      # Información de objetos
├── ride/        # Gestión de atracciones
└── world/       # Manipulación del mundo
```

---

## 8. Internacionalización (i18n)

### 8.1 Sistema de Localización

- Archivos en `data/language/`
- Formato: `{código-idioma}.txt`
- Base: `en-GB.txt` (inglés británico)
- Soporte para texto RTL (Right-to-Left)
- Formateo de números, fechas y moneda

### 8.2 Idiomas Soportados

Más de 50 idiomas incluyendo:
- Europeos: Español, Francés, Alemán, Italiano, etc.
- Asiáticos: Chino, Japonés, Coreano
- Otros: Árabe, Hebreo, Ruso, etc.

---

## 9. Construcción y Compilación

### 9.1 Requisitos

- **Compilador**: GCC 12+ o equivalente
- **Estándar**: C++20
- **CMake**: 3.24+
- **Dependencias principales**: SDL2, OpenGL/DirectX, libpng, zlib, etc.

### 9.2 Opciones de Compilación

```cmake
WITH_TESTS                    # Compilar pruebas
PORTABLE                      # Build portable (-rpath=$ORIGIN)
DOWNLOAD_TITLE_SEQUENCES      # Descargar secuencias de título
DOWNLOAD_OBJECTS              # Descargar objetos
DOWNLOAD_OPENSFX              # Descargar efectos de sonido
DOWNLOAD_OPENMUSIC            # Descargar música
DISABLE_NETWORK               # Deshabilitar multijugador
ENABLE_SCRIPTING              # Habilitar scripting
```

### 9.3 Plataformas Soportadas

- Windows (Win32, UWP)
- Linux
- macOS
- Android
- Emscripten (WebAssembly)

---

## 10. Pruebas

### 10.1 Tests Unitarios

Directorio: `test/tests/`
- Framework de testing personalizado
- Pruebas de funcionalidades críticas
- Validación de compatibilidad con RCT2

---

## 11. Actores y Roles del Proyecto

### 11.1 Equipo de Desarrollo

Según `contributors.md`:
- **Core Team**: Mantenedores principales
- **Contributors**: Desarrolladores que envían PRs
- **Traductores**: Equipo de localización
- **Artistas**: Gráficos y assets
- **Comunidad**: Reportes de bugs, sugerencias

### 11.2 Cómo Contribuir

1. **Bug fixes**: Reportar/encontrar bugs en issue tracker
2. **Nuevas features**: Coordinar con el equipo vía Discord
3. **Traducción**: Repositorio separado OpenRCT2/Localisation
4. **Gráficos**: Proyecto OpenGraphics
5. **Audio**: Proyecto OpenMusic
6. **Escenarios**: Repositorio OpenScenarios

---

## 12. Políticas y Licencias

### 12.1 Licencia

- **GNU GPL v3.0 o superior**
- Requiere original RCT2 para jugar (assets propietarios)

### 12.2 Políticas

- **Código de Conducta**: Aplica a todos los repositorios
- **Code Signing**: SignPath Foundation para releases oficiales
- **Privacidad**: Ver PRIVACY.md

---

## 13. Servicios Externos

### 13.1 Infraestructura

| Servicio | Proveedor | Propósito |
|----------|-----------|-----------|
| Hosting | DigitalOcean | Servicios web |
| IDE | JetBrains | CLion y herramientas |
| Crash Reports | Backtrace | Análisis de minidumps |
| Code Signing | SignPath | Firmado de releases |

### 13.2 Comunicación

- **Discord**: Canales por idioma y propósito
  - Development: #development
  - Localización: #localisation
  - Help: #help
  - Asset creation: #open-graphics, #open-sound-and-music

---

## 14. Características Destacadas vs RCT2 Original

### 14.1 Mejoras Implementadas

- ✅ Soporte multiplataforma
- ✅ Multijugador cooperativo
- ✅ Límites aumentados (más guests, rides, tiles)
- ✅ Herramientas de edición mejoradas
- ✅ Scripting API
- ✅ Temas personalizables
- ✅ Secuencias de título personalizadas
- ✅ Replays y snapshots de estado
- ✅ Herramientas de debug
- ✅ Calidad de vida: atajos, zoom, rotación libre

### 14.2 Mecánicas Re-introducidas de RCT1

- Mountain tool in-game
- Objetivo "have fun"
- Coasters lanzados (sin pasar por estación)
- Botones adicionales en toolbar

---

## 15. Puntos de Entrada Principales

### 15.1 Archivos Clave para Entender el Proyecto

```
src/openrct2/OpenRCT2.cpp          # Entry point principal
src/openrct2/Context.cpp           # Contexto y coordinación
src/openrct2/Game.cpp              # Lógica principal del juego
src/openrct2/GameState.cpp         # Estado del juego
src/openrct2/actions/GameActionRunner.cpp  # Ejecutor de acciones
src/openrct2/world/Map.cpp         # Sistema de mapa
src/openrct2/entity/EntityRegistry.cpp     # Registro de entidades
src/openrct2/network/NetworkBase.cpp       # Base de red
src/openrct2/scripting/ScriptEngine.cpp    # Motor de scripting
src/openrct2-ui/UiContext.cpp      # Contexto de UI
```

---

## 16. Recomendaciones para Nuevos Desarrolladores

1. **Leer wiki**: https://github.com/OpenRCT2/OpenRCT2/wiki
2. **Unirse a Discord**: Canal #development para preguntas
3. **Empezar con bugs pequeños**: Issue tracker etiquetado
4. **Seguir estilo de código**: Wiki/Coding-Style
5. **Usar branch develop**: Nunca hacer PR directo a master
6. **Probar cambios**: Compilar y ejecutar tests

---

## 17. Glosario de Términos

| Término | Significado |
|---------|-------------|
| **Peep** | Personaje/guest/staff del juego |
| **Tile** | Cuadrícula básica del mapa (32x32) |
| **Tile Element** | Elemento dentro de un tile |
| **Ride** | Atracción del parque |
| **Scenario** | Escenario con objetivos específicos |
| **Object** | Asset del juego (ride, scenery, etc.) |
| **Action** | Comando ejecutable en el juego |
| **Sprite** | Imagen 2D renderizada |
| **Viewport** | Vista/rendering del juego |

---

## 18. Mejoras de Seguridad y Física Implementadas (2024)

### 18.1 Sistema de Bloques Inteligente para Montañas Rusas

**Archivos Modificados/Creados:**
- `src/openrct2/ride/Vehicle.NPCReaction.cpp` (nuevo - 271 líneas)
- `src/openrct2/ride/Vehicle.TrackMotion.cpp` (modificado)
- `src/openrct2/ride/Vehicle.h` (modificado - declaraciones de métodos)

**Características Implementadas:**

#### A. Bloques Dinámicos con Verificación de Distancia de Seguridad
- **Función**: `Vehicle::CheckBlockSectionSafetyDistance(const Vehicle* precedingVehicle)`
- **Ubicación**: `Vehicle.NPCReaction.cpp` líneas 175-231
- **Lógica**:
  - Calcula distancia 3D entre vehículos consecutivos
  - Determina distancia mínima segura basada en velocidad actual
  - Considera margen adicional para coasters de alta velocidad (>60 MPH)
  - Verifica velocidad relativa para detectar acercamiento peligroso
  - Retorna `false` si se requiere frenado de emergencia

#### B. Frenado de Emergencia Progresivo
- **Función**: `Vehicle::ApplyProgressiveEmergencyBraking(int32_t distanceToVehicle, int32_t safeDistance)`
- **Ubicación**: `Vehicle.NPCReaction.cpp` líneas 241-271
- **Lógica**:
  - Calcula ratio de urgencia (0.0 a 1.0) basado en proximidad al peligro
  - Aplica fuerza de frenado progresiva según criticidad
  - Frenado mínimo cuando está ligeramente por debajo de distancia segura
  - Frenado máximo cuando la colisión es inminente (<25% de distancia segura)
  - Previene movimiento hacia atrás durante frenado
  - Setea flag `VEHICLE_UPDATE_MOTION_TRACK_FLAG_VEHICLE_AT_BLOCK_BRAKE` en situaciones críticas

#### C. Integración en Loop Principal de Movimiento
- **Ubicación**: `Vehicle.TrackMotion.cpp` líneas 289-315
- **Ejecución**: 
  - Solo se activa en rides con block sections habilitadas (`curRide->isBlockSectioned()`)
  - Solo verifica el vehículo líder de cada tren (`IsHead()`)
  - Se ejecuta durante estado `travelling`
  - Calcula distancias y aplica frenado progresivo automáticamente

**Beneficios:**
- ✅ Previene colisiones entre trenes en block sections
- ✅ Frenado suave y progresivo en lugar de paradas bruscas
- ✅ Mantiene precisión de física original (no modifica cálculos de velocidad/aceleración base)
- ✅ Adaptable a diferentes tipos de coasters y velocidades

### 18.2 Sistema de Reacciones de NPCs a Fuerzas G

**Archivo Creado:** `src/openrct2/ride/Vehicle.NPCReaction.cpp`

**Características Implementadas:**

#### A. Actualización de Reacciones a G-Forces
- **Función**: `Vehicle::UpdateRiderReactionsToGForces(const GForces& gForces)`
- **Ubicación**: `Vehicle.NPCReaction.cpp` líneas 27-42
- **Integración**: Llamado desde `Vehicle::UpdateTrackMotionUpStopCheck()` (línea 96)
- **Condiciones de Activación**:
  - Vehículo debe tener pasajeros (`num_peeps > 0`)
  - Ride debe ser válido
  - Estado del vehículo: `travelling`, `departing`, o `arriving`

#### B. Aplicación de Efectos de Fuerzas G a Riders
- **Función**: `Vehicle::ApplyGForceEffectsToRiders(const GForces& gForces)`
- **Ubicación**: `Vehicle.NPCReaction.cpp` líneas 49-161

**Efectos por Tipo de Fuerza G:**

| Tipo de G-Fuerza | Rango | Efectos en NPCs |
|------------------|-------|-----------------|
| **Vertical Positiva Alta** | >160 (>2G) | +8 Nausea, +4 Excitement |
| **Vertical Positiva Extrema** | >320 (>4G) | +12 Nausea, -5 Energy, 6.25% chance de vómito, pensamiento "intensify" |
| **Vertical Negativa Leve** | <-40 (<-0.5G) | +8 Excitement (airtime) |
| **Vertical Negativa Fuerte** | <-80 (<-1G) | +12 Excitement, +6 Nausea, pensamiento "great" |
| **Vertical Negativa Muy Fuerte** | <-160 (<-2G) | +10 Nausea, 50% chance de pensamiento "scream" |
| **Lateral Alta** | >100 | +6 Nausea |
| **Lateral Muy Alta** | >200 | +10 Nausea, +3 Excitement, pensamiento "rough" |
| **Normal** | -40 a 160 | +2 Excitement ocasional |

**Mecánica de Pensamientos:**
- Usa combinación de `peep->PeepId + getGameState().currentTicks` para aleatoriedad determinista
- Probabilidades calculadas con módulo para asegurar reproducibilidad en multijugador
- Tipos de pensamientos: `vomiting`, `intensify`, `great`, `scream`, `rough`

**Beneficios:**
- ✅ NPCs reaccionan dinámicamente a la experiencia de la montaña rusa
- ✅ Aumenta realismo e inmersión sin afectar física base
- ✅ Sistema determinista compatible con multijugador
- ✅ Diferenciación entre tipos de fuerzas G (verticales vs laterales)
- ✅ Efectos acumulativos en náusea y excitación

### 18.3 Impacto en el Proyecto

**Archivos Involucrados:**
```
src/openrct2/ride/
├── Vehicle.h                    # Declaraciones de métodos (2 nuevas funciones miembro)
├── Vehicle.NPCReaction.cpp      # NUEVO - Lógica de reacciones y block sections (271 líneas)
└── Vehicle.TrackMotion.cpp      # Integración de verificaciones (modificado ~30 líneas)
```

**Integración con CMake:**
- El sistema usa `file(GLOB_RECURSE OPENRCT2_CORE_SOURCES "*.cpp")` en `src/openrct2/CMakeLists.txt`
- Los nuevos archivos `.cpp` se incluyen automáticamente sin modificar CMakeLists
- No requiere cambios en configuración de build

**Puntos de Consumo/Integración:**
1. `Vehicle.TrackMotion.cpp::UpdateTrackMotionUpStopCheck()` → llama a `UpdateRiderReactionsToGForces()`
2. `Vehicle.TrackMotion.cpp::CheckAndApplyBlockSectionStopSite()` → integra verificación de distancia y frenado progresivo
3. Sistema de entidades → actualiza stats de Peeps (NauseaTarget, ExcitementTarget, EnergyTarget)
4. Sistema de pensamientos → inserta thoughts basados en condiciones de G-forces

**QA Realizado:**
- ✅ Verificación de inclusión automática en CMake (GLOB_RECURSE)
- ✅ Confirmación de firmas de métodos en Vehicle.h
- ✅ Validación de integración en flujo existente de TrackMotion
- ✅ Verificación de no-modificación de cálculos físicos base
- ✅ Confirmación de compatibilidad con sistema determinista (multijugador)

**Consideraciones de Rendimiento:**
- Cálculos de distancia usan aproximación Manhattan (dx+dy+dz/2) en lugar de sqrt para eficiencia
- Verificaciones solo se ejecutan para vehículos líderes en block sections
- Efectos en NPCs procesados solo cuando vehículo está en movimiento
- Aleatoriedad determinista evita overhead de generación de números aleatorios

### 18.4 Próximas Mejoras Planificadas (Roadmap)

Basado en las 16 mejoras identificadas, las siguientes prioridades son:

**Prioridad Alta:**
1. ✅ **Sistema de Bloques Inteligente** - COMPLETADO
2. ⏳ **Reacciones Realistas de NPCs** - PARCIALMENTE COMPLETADO (falta animaciones procedurales)
3. ⏳ **Sistema de Colisiones Universal** - Pendiente (actualmente solo dodgems y boat hire)

**Prioridad Media:**
4. ⏳ **Tipos de Vía Nuevos** - Pendiente (transferencias automáticas, switches, LSM/IM)
5. ⏳ **Señalización en Tiempo Real** - Pendiente (luces/sonidos de estado de bloques)

**Prioridad Baja (Gameplay/Environment):**
6-16. Características adicionales de jugabilidad, ambiente y herramientas de desarrollo

---

## Conclusión

OpenRCT2 es un proyecto maduro y bien estructurado que ha logrado reverse-engineer completamente RCT2 mientras añade numerosas mejoras modernas. Su arquitectura modular facilita la extensión y mantenimiento. El código sigue patrones modernos de C++ con separación clara de responsabilidades entre lógica, renderizado, UI y datos.

**Fortalezas:**
- Código base limpio y documentado
- Comunidad activa y organizada
- Multiplataforma real
- Extensible vía scripting
- Compatible con contenido original
- **NUEVO**: Sistema de seguridad de block sections con frenado progresivo
- **NUEVO**: Reacciones dinámicas de NPCs a fuerzas G

**Áreas de atención:**
- Curva de aprendizaje por tamaño del código
- Dependencia de assets originales
- Complejidad del sistema de red determinista
- **EN PROGRESO**: Animaciones procedurales para NPCs
- **PENDIENTE**: Sistema de colisiones universal

**Mejoras Recientes (2024):**
- Implementación de sistema inteligente de block sections previene colisiones
- NPCs ahora reaccionan dinámicamente a fuerzas G con pensamientos y cambios de estado
- Frenado de emergencia progresivo mejora realismo y seguridad
- Todo sin comprometer precisión de cálculos físicos originales

---

*Documento generado para servir como referencia base en futuras conversaciones sobre el proyecto OpenRCT2.*
*Fecha: 2024*
*Versión del análisis: 2.0 - Con mejoras de seguridad y física implementadas*
*Última actualización: Implementación de Sistema de Bloques Inteligente y Reacciones de NPCs*
