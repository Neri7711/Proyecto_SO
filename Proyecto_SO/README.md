# MiniShell - Intérprete de Comandos en C++

Un proyecto de Sistemas Operativos que implementa una mini shell en C++ para sistemas Linux, utilizando llamadas al sistema POSIX.

## Características Implementadas

### Funcionalidades Básicas
- **Ejecución de comandos externos** mediante `fork()` y `execvp()`
- **Comandos integrados (built-in)**: `cd`, `pwd`, `exit`, `help`
- **Manejo de señales**: Ctrl+C (SIGINT) y Ctrl+Z (SIGTSTP)

### Características Avanzadas
- **Redirección de entrada/salida**:
  - `< archivo` - Redirigir entrada desde archivo
  - `> archivo` - Redirigir salida a archivo (sobreescribir)
  - `>> archivo` - Redirigir salida a archivo (append)

- **Pipes (comunicación entre procesos)**:
  - `comando1 | comando2` - Conectar salida de comando1 con entrada de comando2
  - Soporte para múltiples pipes: `cmd1 | cmd2 | cmd3`

- **Procesos en segundo plano**:
  - `comando &` - Ejecutar comando en background
  - Monitoreo automático de procesos terminados

## Compilación

```bash
# Compilar el proyecto
make

# Limpiar archivos compilados
make clean

# Instalar en el sistema (opcional)
sudo make install
```

## Uso

```bash
# Ejecutar la shell
./minishell
```

### Ejemplos de Uso

```bash
# Comandos básicos
ls -la
pwd
cd /tmp

# Redirección
ls -la > archivo.txt
cat < archivo.txt
echo "Hola mundo" >> salida.txt

# Pipes
ls -la | grep ".cpp"
cat archivo.txt | wc -l
ps aux | grep minishell | head -5

# Procesos en segundo plano
sleep 10 &
ping google.com > ping_output.txt &
```

## Estructura del Código

### Clase MiniShell
- **Constructor**: Inicializa manejadores de señales
- **run()**: Bucle principal de la shell
- **tokenize()**: Parseo de línea de comandos
- **execute_command()**: Ejecución de comandos simples
- **execute_pipeline()**: Manejo de pipes entre comandos
- **handle_redirection()**: Procesamiento de redirecciones
- **cleanup_background()**: Limpieza de procesos en background

### Llamadas al Sistema Utilizadas
- `fork()` - Creación de procesos
- `execvp()` - Ejecución de programas
- `waitpid()` - Espera de procesos hijos
- `pipe()` - Creación de pipes
- `dup2()` - Duplicación de descriptores de archivo
- `open()` - Apertura de archivos
- `signal()` - Manejo de señales
- `chdir()` - Cambio de directorio
- `getcwd()` - Obtener directorio actual

## Requisitos del Sistema

- Sistema operativo Linux
- Compilador g++ con soporte C++11
- Make para compilación

## Conceptos de Sistemas Operativos Demostrados

1. **Creación y Gestión de Procesos**
   - Uso de `fork()` para crear procesos hijos
   - Diferenciación entre procesos padre e hijo
   - Espera activa y pasiva de procesos

2. **Comunicación Entre Procesos (IPC)**
   - Pipes para comunicación unidireccional
   - Redirección de entrada/salida estándar
   - Encadenamiento de múltiples procesos

3. **Manejo de Descriptores de Archivo**
   - Duplicación con `dup2()`
   - Redirección de stdin/stdout
   - Gestión de archivos de entrada/salida

4. **Sincronización y Control**
   - Espera de procesos con `waitpid()`
   - Procesos en segundo plano
   - Manejo de señales asíncronas

5. **Interfaz de Usuario**
   - Prompt personalizado con directorio actual
   - Parseo de línea de comandos
   - Manejo integrado de comandos

## Limitaciones Conocidas

- No soporta expansión de variables de entorno
- No implementa historial de comandos
- No soporta scripting (archivos batch)
- No manejo de trabajos (jobs) avanzado
- Sin autocompletado con TAB

## Posibles Mejoras

1. Implementar historial de comandos (flechas arriba/abajo)
2. Agregar más comandos built-in (alias, export, etc.)
3. Soporte para variables de entorno
4. Implementar control de trabajos (fg, bg, jobs)
5. Agregar autocompletado con TAB
6. Soporte para scripting básico

## Autor

Proyecto desarrollado para la materia de Sistemas Operativos.
Demuestra implementación práctica de conceptos fundamentales de SO desde cero en C++.
