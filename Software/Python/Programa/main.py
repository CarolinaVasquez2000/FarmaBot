# Importar las bibliotecas necesarias
import tkinter as tk
from tkinter import messagebox, Listbox, PhotoImage
import cv2
import numpy as np
import threading
from PIL import Image, ImageTk
import requests
from pyzbar.pyzbar import decode
import csv
import time

# URL base del ESP32-CAM (cambiar por la IP correcta)
ip_base = "http://192.168.251.229"

# Endpoints del ESP32-CAM para distintas acciones
url_arranque = f"{ip_base}/arranque"
url_imagen = f"{ip_base}/jpg"
url_torre = f"{ip_base}/torre"
url_baja = f"{ip_base}/baja"

# Variable para almacenar el último QR detectado y evitar lecturas repetidas
last_qr_data = None

# Función para cargar medicamentos desde un archivo CSV
def cargar_medicamentos(filename):
    medicamentos = []
    with open(filename, newline='') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            medicamentos.append((row[0], row[1]))  # Agrega (nombre, ubicación)
    return medicamentos

# Función para calcular posición en píxeles a partir de porcentajes de la pantalla
def calcular_posicion(porcentaje_x, porcentaje_y, ancho_pantalla, alto_pantalla):
    return int(porcentaje_x * ancho_pantalla), int(porcentaje_y * alto_pantalla)

# Función para filtrar medicamentos según texto ingresado
def filtrar_medicamentos(event=None):
    search_term = entry_buscar.get().lower()
    listbox_medicamentos.delete(0, tk.END)
    for medicamento in medicamentos:
        if search_term in medicamento[0].lower():
            listbox_medicamentos.insert(tk.END, medicamento[0])

# Función para manejar la selección de medicamento
def mostrar_medicamento_seleccionado():
    global letra, numero
    seleccionado = listbox_medicamentos.curselection()
    if seleccionado:
        nombre_medicamento = listbox_medicamentos.get(seleccionado)
        for medicamento in medicamentos:
            if medicamento[0] == nombre_medicamento:
                ubicacion = medicamento[1]
                letra, numero = ubicacion.split("-")
                try:
                    # Enviar señal de arranque al ESP32-CAM
                    control_response = requests.get(url_arranque, params={"state": "1"})
                    if control_response.status_code == 200:
                        label_msg.config(text="Arranque de recorrido")
                        pos_label_msg = calcular_posicion(0.5, 0.2, ancho_pantalla, alto_pantalla)
                        label_msg.place(x=pos_label_msg[0] - label_msg.winfo_reqwidth() // 2, y=pos_label_msg[1])
                except Exception as e:
                    print(f"Error al enviar la solicitud: {e}")
                break

# Función para mostrar transmisión de video y leer QR
def mostrar_video():
    global last_qr_data

    # Función interna para intentar conexión y procesar imagen
    def intentar_conexion():
        global last_qr_data
        try:
            # Solicitar imagen al ESP32-CAM
            response = requests.get(url_imagen, stream=True, timeout=2)
            if response.status_code == 200:
                img_array = np.asarray(bytearray(response.content), dtype=np.uint8)
                img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)

                # Detectar códigos QR
                decoded_objects = decode(img)
                for obj in decoded_objects:
                    qr_data = obj.data.decode('utf-8')

                    # Procesar solo si el QR es nuevo
                    if qr_data != last_qr_data:
                        last_qr_data = qr_data
                        label_msg.config(text=f"Posición buscada: {letra} - QR detectado: {qr_data}")
                        pos_label_msg = calcular_posicion(0.5, 0.2, ancho_pantalla, alto_pantalla)
                        label_msg.place(x=pos_label_msg[0] - label_msg.winfo_reqwidth() // 2, y=pos_label_msg[1])

                        # Si detecta QR "0", detener recorrido
                        if qr_data == "0":
                            try:
                                control_response = requests.get(url_arranque, params={"state": "0"})
                                label_msg.config(text="Recorrido finalizado")
                                label_msg.place(x=pos_label_msg[0] - label_msg.winfo_reqwidth() // 2, y=pos_label_msg[1])
                            except Exception as e:
                                print(f"Error al enviar la solicitud: {e}")

                        # Si QR coincide con la letra esperada
                        if qr_data == letra:
                            try:
                                if numero == "01":
                                    requests.get(url_baja, params={"state": "0"})  # Baja altura
                                elif numero == "02":
                                    requests.get(url_baja, params={"state": "1"})  # Sube altura
                                else:
                                    print("La altura no es accesible para el robot")

                                time.sleep(0.5)  # Espera para estabilizar

                                requests.get(url_torre, params={"state": "1"})  # Indica QR detectado
                                label_msg.config(text="QR de ubicación detectada")

                            except Exception as e:
                                print(f"Error al enviar la solicitud: {e}")

                        else:
                            print("La ubicación no es accesible para el robot")

                    # Dibujar contorno del QR en la imagen
                    points = obj.polygon
                    if len(points) == 4:
                        pts = np.array(points, dtype=np.int32).reshape((-1, 1, 2))
                        cv2.polylines(img, [pts], True, (0, 255, 0), 2)
                    else:
                        cv2.drawContours(img, [np.array(points, dtype=np.int32)], -1, (0, 255, 0), 2)

                # Convertir a formato compatible con Tkinter
                img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
                img_pil = Image.fromarray(img_rgb)
                img_tk = ImageTk.PhotoImage(image=img_pil)

                # Mostrar imagen en la GUI
                label_video.config(image=img_tk)
                label_video.image = img_tk

                # Repetir ciclo
                label_video.after(10, mostrar_video)
            else:
                print(f"❌ Falló la respuesta: código {response.status_code}")
                reconectar()

        except Exception as e:
            print(f"⚠️ Error al obtener la imagen: {e}")
            reconectar()

    # Función para reconectar si falla la imagen
    def reconectar():
        global last_qr_data
        last_qr_data = None
        label_msg.config(text="Reconectando con el ESP32...")
        pos_label_msg = calcular_posicion(0.5, 0.2, ancho_pantalla, alto_pantalla)
        label_msg.place(x=pos_label_msg[0] - label_msg.winfo_reqwidth() // 2, y=pos_label_msg[1])
        label_video.after(3000, mostrar_video)  # Reintentar en 3 segundos

    # Ejecutar la conexión inicial
    intentar_conexion()

# Crear ventana principal
root = tk.Tk()
root.title("FarmaBot")
root.resizable(False, False)
root.state('zoomed')  # Abrir maximizada
root.configure(bg='white')

# Obtener resolución de pantalla
ancho_pantalla = root.winfo_screenwidth()
alto_pantalla = root.winfo_screenheight()

# Cargar medicamentos desde CSV
medicamentos = cargar_medicamentos('C:\\Users\\matia\\OneDrive\\Escritorio\\Proyecto Ingeneria Mecatronica\\Scripts\\Scripts Carro\\Python\\Interfaz grafica\\medicamentos.csv')

# Cargar logo o imagen lateral
imagen_izquierda = PhotoImage(file="C:\\Users\\matia\\OneDrive\\Escritorio\\Proyecto Ingeneria Mecatronica\\Scripts\\Scripts Carro\\Python\\Interfaz grafica\\logo.png")

# Crear etiqueta para el video
label_video = tk.Label(root, bg='white', bd=0, highlightthickness=0)

# Crear y configurar widgets de la interfaz
label_titulo = tk.Label(root, text="FarmaBot", font=("Helvetica", 40, "bold"), anchor='center', fg='#3B24C8', bg='white')
label_msg = tk.Label(root, text="Seleccione un medicamento", font=("Helvetica", 30, "bold"), anchor='center', fg='red', bg='white')
label_buscar = tk.Label(root, text="Buscar:", font=("Helvetica", 25, "bold"), anchor='center', bg='white')
entry_buscar = tk.Entry(root, font=("Helvetica", 25, "bold"), bg='#868686')
entry_buscar.bind('<KeyRelease>', filtrar_medicamentos)
listbox_medicamentos = Listbox(root, font=("Helvetica", 25, "bold"), bg='#868686')
button_seleccionar = tk.Button(root, text="Seleccionar", font=("Helvetica", 20, "bold"), bg='#6B7CFF', command=mostrar_medicamento_seleccionado)

# Imagen superior izquierda
label_imagen_izquierda = tk.Label(root, image=imagen_izquierda, bg='white')

# Ubicar etiqueta de video a la derecha
pos_video = calcular_posicion(0.9, 0.2, ancho_pantalla, alto_pantalla)
label_video.place(x=pos_video[0] - 320, y=pos_video[1] - 240, width=640, height=480)

# Posicionar título
pos_label_titulo = calcular_posicion(0.5, 0.1, ancho_pantalla, alto_pantalla)
label_titulo.place(x=pos_label_titulo[0] - label_titulo.winfo_reqwidth() // 2, y=pos_label_titulo[1])

# Posicionar mensaje
pos_label_msg = calcular_posicion(0.5, 0.2, ancho_pantalla, alto_pantalla)
label_msg.place(x=pos_label_msg[0] - label_msg.winfo_reqwidth() // 2, y=pos_label_msg[1])

# Posicionar campo de búsqueda y lista
pos_label_buscar = calcular_posicion(0.1, 0.3, ancho_pantalla, alto_pantalla)
label_buscar.place(x=pos_label_buscar[0], y=pos_label_buscar[1])

pos_entry_buscar = calcular_posicion(0.2, 0.3, ancho_pantalla, alto_pantalla)
entry_buscar.place(x=pos_entry_buscar[0], y=pos_entry_buscar[1], width=500)

pos_listbox = calcular_posicion(0.2, 0.4, ancho_pantalla, alto_pantalla)
listbox_medicamentos.place(x=pos_listbox[0], y=pos_listbox[1], width=800, height=350)

# Botón de selección
pos_button = calcular_posicion(0.65, 0.29, ancho_pantalla, alto_pantalla)
button_seleccionar.place(x=pos_button[0], y=pos_button[1])

# Posicionar logo superior izquierdo
label_imagen_izquierda.place(x=10, y=10)

# Lanzar hilo para mostrar video sin bloquear interfaz
threading.Thread(target=mostrar_video, daemon=True).start()

# Inicializar lista filtrada
filtrar_medicamentos()

# Ejecutar bucle principal de la GUI
root.mainloop()
