"""STI Editor -- desktop viewer/editor for JA2 .sti files (Tk + Pillow).

Run:  py -3 tools/sti_editor/sti_editor.py [file.sti]
See README.md for the feature list and the format's limitations.
"""

from __future__ import annotations

import os
import sys
import tkinter as tk
from tkinter import colorchooser, filedialog, messagebox, simpledialog, ttk

from PIL import Image, ImageDraw, ImageTk

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import drawing  # noqa: E402
import palette_io  # noqa: E402
import sti  # noqa: E402

APP_TITLE = "STI Editor"
ZOOM_LEVELS = [1, 2, 3, 4, 6, 8, 12, 16, 24, 32]
MAX_RENDER_PIXELS = 24_000_000   # cap on the zoomed image size (memory)
UNDO_LIMIT = 100
PALETTE_CELL = 14

TOOLS = [
    ("pencil", "Ołówek", "P"),
    ("line", "Linia", "L"),
    ("rect", "Prostokąt", "R"),
    ("ellipse", "Elipsa", "E"),
    ("fill", "Wypełnienie", "F"),
    ("text", "Tekst", "T"),
    ("picker", "Pipeta", "I"),
    ("eraser", "Gumka (przezroczystość)", "X"),
]

BACKGROUNDS = {
    "checker": "Szachownica",
    "black": "Czarne",
    "magenta": "Magenta",
    "white": "Białe",
}


class Snapshot:
    """One undo step: a frame's state, the palette, or the frame list."""

    def __init__(self, kind, index=None, data=None, current=None):
        self.kind, self.index, self.data, self.current = kind, index, data, current


class STIEditor:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.sti: sti.STIFile | None = None
        self.path: str | None = None
        self.dirty = False
        self.frame_idx = 0
        self.zoom = 4
        self.color: int | None = 1          # selected palette index; None = transparent
        self.undo_stack: list[Snapshot] = []
        self.redo_stack: list[Snapshot] = []
        self.playing = False
        self._play_job = None
        self._render_job = None
        self._photo = None
        self._drag_start = None
        self._stroke = []
        self._preview_mask = None

        self.tool = tk.StringVar(value="pencil")
        self.filled = tk.BooleanVar(value=False)
        self.brush = tk.IntVar(value=1)
        self.font_size = tk.IntVar(value=12)
        self.background = tk.StringVar(value="checker")
        self.show_mask = tk.BooleanVar(value=False)
        self.show_grid = tk.BooleanVar(value=False)
        self.delay = tk.IntVar(value=100)
        self.frame_var = tk.StringVar(value="0")

        root.title(APP_TITLE)
        root.geometry("1280x800")
        root.protocol("WM_DELETE_WINDOW", self.quit)
        self._build_menu()
        self._build_ui()
        self._bind_keys()
        self._update_all()

    # ------------------------------------------------------------------ UI
    def _build_menu(self):
        m = tk.Menu(self.root)
        f = tk.Menu(m, tearoff=False)
        f.add_command(label="Otwórz…", accelerator="Ctrl+O", command=self.open_file)
        f.add_command(label="Nowy z PNG…", command=self.new_from_png)
        f.add_command(label="Zapisz", accelerator="Ctrl+S", command=self.save)
        f.add_command(label="Zapisz jako…", command=self.save_as)
        f.add_separator()
        f.add_command(label="Eksport klatki do PNG…", command=self.export_frame)
        f.add_command(label="Eksport wszystkich klatek do PNG…", command=self.export_all)
        f.add_command(label="Eksport maski klatki do PNG…", command=self.export_mask)
        f.add_command(label="Import PNG do bieżącej klatki…", command=self.import_png)
        f.add_separator()
        f.add_command(label="Wyjście", command=self.quit)
        m.add_cascade(label="Plik", menu=f)

        e = tk.Menu(m, tearoff=False)
        e.add_command(label="Cofnij", accelerator="Ctrl+Z", command=self.undo)
        e.add_command(label="Ponów", accelerator="Ctrl+Y", command=self.redo)
        e.add_separator()
        e.add_command(label="Duplikuj klatkę", command=self.duplicate_frame)
        e.add_command(label="Nowa pusta klatka…", command=self.new_frame)
        e.add_command(label="Usuń klatkę", command=self.delete_frame)
        e.add_command(label="Wyczyść klatkę (przezroczysta)", command=self.clear_frame)
        m.add_cascade(label="Edycja", menu=e)

        v = tk.Menu(m, tearoff=False)
        v.add_command(label="Powiększ", accelerator="Ctrl++", command=lambda: self.change_zoom(+1))
        v.add_command(label="Pomniejsz", accelerator="Ctrl+-", command=lambda: self.change_zoom(-1))
        v.add_separator()
        for key, label in BACKGROUNDS.items():
            v.add_radiobutton(label=f"Tło: {label}", variable=self.background, value=key, command=self.render)
        v.add_separator()
        v.add_checkbutton(label="Podgląd maski przezroczystości", variable=self.show_mask, command=self.render)
        v.add_checkbutton(label="Siatka pikseli (zoom ≥ 6)", variable=self.show_grid, command=self.render)
        m.add_cascade(label="Widok", menu=v)

        p = tk.Menu(m, tearoff=False)
        p.add_command(label="Importuj paletę…", command=self.import_palette)
        p.add_command(label="Eksportuj paletę…", command=self.export_palette)
        m.add_cascade(label="Paleta", menu=p)

        a = tk.Menu(m, tearoff=False)
        a.add_command(label="Analiza bieżącej klatki", command=self.analyze_frame)
        a.add_command(label="Informacje o pliku", command=self.file_info)
        m.add_cascade(label="Analiza", menu=a)
        self.root.config(menu=m)

    def _build_ui(self):
        main = ttk.Frame(self.root)
        main.pack(fill="both", expand=True)

        # tools (left)
        left = ttk.Frame(main, padding=4)
        left.pack(side="left", fill="y")
        ttk.Label(left, text="Narzędzia").pack(anchor="w")
        for key, label, short in TOOLS:
            ttk.Radiobutton(left, text=f"{label} ({short})", value=key, variable=self.tool).pack(anchor="w")
        ttk.Checkbutton(left, text="Wypełniony kształt", variable=self.filled).pack(anchor="w", pady=(8, 0))
        row = ttk.Frame(left)
        row.pack(anchor="w", pady=2)
        ttk.Label(row, text="Pędzel:").pack(side="left")
        ttk.Spinbox(row, from_=1, to=16, width=4, textvariable=self.brush).pack(side="left")
        row = ttk.Frame(left)
        row.pack(anchor="w", pady=2)
        ttk.Label(row, text="Czcionka:").pack(side="left")
        ttk.Spinbox(row, from_=6, to=72, width=4, textvariable=self.font_size).pack(side="left")
        ttk.Separator(left).pack(fill="x", pady=6)
        row = ttk.Frame(left)
        row.pack(anchor="w")
        ttk.Button(row, text="−", width=3, command=lambda: self.change_zoom(-1)).pack(side="left")
        self.zoom_label = ttk.Label(row, text="", width=6, anchor="center")
        self.zoom_label.pack(side="left")
        ttk.Button(row, text="+", width=3, command=lambda: self.change_zoom(+1)).pack(side="left")

        # right: palette + info
        right = ttk.Frame(main, padding=4)
        right.pack(side="right", fill="y")
        ttk.Label(right, text="Paleta (klik: wybór, 2×klik: edycja koloru)").pack(anchor="w")
        self.pal_canvas = tk.Canvas(right, width=16 * PALETTE_CELL + 1, height=16 * PALETTE_CELL + 1,
            highlightthickness=0, bg="#202020")
        self.pal_canvas.pack(anchor="w")
        self.pal_canvas.bind("<Button-1>", self._palette_click)
        self.pal_canvas.bind("<Double-Button-1>", self._palette_edit)
        self.pal_canvas.bind("<Motion>", self._palette_hover)
        row = ttk.Frame(right)
        row.pack(anchor="w", fill="x", pady=4)
        self.swatch = tk.Canvas(row, width=36, height=24, highlightthickness=1)
        self.swatch.pack(side="left")
        self.color_label = ttk.Label(row, text="", width=28)
        self.color_label.pack(side="left", padx=4)
        ttk.Button(right, text="Przezroczysty (gumka)", command=lambda: self.select_color(None)).pack(anchor="w")
        row = ttk.Frame(right)
        row.pack(anchor="w", pady=(2, 0))
        ttk.Button(row, text="Importuj paletę…", command=self.import_palette).pack(side="left")
        ttk.Button(row, text="Eksportuj paletę…", command=self.export_palette).pack(side="left", padx=(4, 0))

        ttk.Separator(right).pack(fill="x", pady=6)
        ttk.Label(right, text="Bieżąca klatka").pack(anchor="w")
        grid = ttk.Frame(right)
        grid.pack(anchor="w", fill="x")
        self.size_label = ttk.Label(grid, text="")
        ttk.Label(grid, text="Rozmiar:").grid(row=0, column=0, sticky="w")
        self.size_label.grid(row=0, column=1, columnspan=3, sticky="w")
        self.off_x = tk.StringVar()
        self.off_y = tk.StringVar()
        ttk.Label(grid, text="Pozycja X/Y:").grid(row=1, column=0, sticky="w")
        ttk.Entry(grid, textvariable=self.off_x, width=7).grid(row=1, column=1, sticky="w")
        ttk.Entry(grid, textvariable=self.off_y, width=7).grid(row=1, column=2, sticky="w")
        ttk.Button(grid, text="Zastosuj", command=self.apply_offset).grid(row=1, column=3, sticky="w")

        self.aux_frame = ttk.LabelFrame(right, text="AuxObjectData (animacja/tile)", padding=4)
        self.aux_frame.pack(anchor="w", fill="x", pady=4)
        self.aux_vars = {}
        for r, (key, label) in enumerate([
                ("number_of_frames", "ubNumberOfFrames"),
                ("current_frame", "ubCurrentFrame"),
                ("flags", "fFlags"),
                ("wall_orientation", "ubWallOrientation"),
                ("number_of_tiles", "ubNumberOfTiles"),
                ("tile_loc_index", "usTileLocIndex")]):
            var = tk.StringVar()
            self.aux_vars[key] = var
            ttk.Label(self.aux_frame, text=label).grid(row=r, column=0, sticky="w")
            ttk.Entry(self.aux_frame, textvariable=var, width=8).grid(row=r, column=1, sticky="w")
        self.aux_flags_label = ttk.Label(self.aux_frame, text="", wraplength=230)
        self.aux_flags_label.grid(row=6, column=0, columnspan=2, sticky="w")
        self.aux_apply = ttk.Button(self.aux_frame, text="Zastosuj aux", command=self.apply_aux)
        self.aux_apply.grid(row=7, column=0, columnspan=2, sticky="w")

        ttk.Separator(right).pack(fill="x", pady=6)
        self.file_label = ttk.Label(right, text="", justify="left", wraplength=240, font=("Consolas", 8))
        self.file_label.pack(anchor="w")

        # centre: canvas
        centre = ttk.Frame(main)
        centre.pack(side="left", fill="both", expand=True)
        self.canvas = tk.Canvas(centre, bg="#3a3a3a", highlightthickness=0, cursor="crosshair")
        hs = ttk.Scrollbar(centre, orient="horizontal", command=self.canvas.xview)
        vs = ttk.Scrollbar(centre, orient="vertical", command=self.canvas.yview)
        self.canvas.configure(xscrollcommand=hs.set, yscrollcommand=vs.set)
        self.canvas.grid(row=0, column=0, sticky="nsew")
        vs.grid(row=0, column=1, sticky="ns")
        hs.grid(row=1, column=0, sticky="ew")
        centre.rowconfigure(0, weight=1)
        centre.columnconfigure(0, weight=1)
        self.canvas.bind("<ButtonPress-1>", self._on_press)
        self.canvas.bind("<B1-Motion>", self._on_drag)
        self.canvas.bind("<ButtonRelease-1>", self._on_release)
        self.canvas.bind("<Button-3>", self._on_pick)
        self.canvas.bind("<Motion>", self._on_motion)
        self.canvas.bind("<Control-MouseWheel>", lambda e: self.change_zoom(+1 if e.delta > 0 else -1))

        # bottom: frame controls
        bottom = ttk.Frame(self.root, padding=4)
        bottom.pack(side="bottom", fill="x")
        ttk.Button(bottom, text="⏮", width=3, command=lambda: self.goto_frame(0)).pack(side="left")
        ttk.Button(bottom, text="◀ Poprz.", command=lambda: self.goto_frame(self.frame_idx - 1)).pack(side="left")
        ttk.Label(bottom, text="Nr obr.:").pack(side="left", padx=(8, 2))
        ent = ttk.Entry(bottom, textvariable=self.frame_var, width=6)
        ent.pack(side="left")
        ent.bind("<Return>", lambda e: self._frame_entry())
        self.count_label = ttk.Label(bottom, text="/ 0")
        self.count_label.pack(side="left", padx=(2, 8))
        ttk.Button(bottom, text="Nast. ▶", command=lambda: self.goto_frame(self.frame_idx + 1)).pack(side="left")
        ttk.Button(bottom, text="⏭", width=3, command=lambda: self.goto_frame(self.frame_count - 1)).pack(side="left")
        self.play_btn = ttk.Button(bottom, text="▶ Odtwórz", command=self.toggle_play)
        self.play_btn.pack(side="left", padx=(16, 2))
        ttk.Label(bottom, text="Czas klatki [ms] (tylko podgląd):").pack(side="left")
        ttk.Spinbox(bottom, from_=10, to=5000, increment=10, width=6, textvariable=self.delay).pack(side="left")
        self.status = ttk.Label(bottom, text="", anchor="e")
        self.status.pack(side="right", fill="x", expand=True)

    def _bind_keys(self):
        r = self.root
        r.bind("<Control-o>", lambda e: self.open_file())
        r.bind("<Control-s>", lambda e: self.save())
        r.bind("<Control-z>", lambda e: self.undo())
        r.bind("<Control-y>", lambda e: self.redo())
        r.bind("<Control-plus>", lambda e: self.change_zoom(+1))
        r.bind("<Control-equal>", lambda e: self.change_zoom(+1))
        r.bind("<Control-minus>", lambda e: self.change_zoom(-1))
        for key, _, short in TOOLS:
            r.bind(f"<KeyPress-{short.lower()}>", lambda e, k=key: self._key_tool(e, k))
        r.bind("<Prior>", lambda e: self.goto_frame(self.frame_idx - 1))
        r.bind("<Next>", lambda e: self.goto_frame(self.frame_idx + 1))
        r.bind("<Left>", lambda e: self._key_frame(e, -1))
        r.bind("<Right>", lambda e: self._key_frame(e, +1))
        r.bind("<space>", lambda e: self._key_play(e))

    def _typing(self, event) -> bool:
        return isinstance(event.widget, (tk.Entry, ttk.Entry, ttk.Spinbox, tk.Spinbox))

    def _key_tool(self, event, key):
        if not self._typing(event):
            self.tool.set(key)

    def _key_frame(self, event, step):
        if not self._typing(event):
            self.goto_frame(self.frame_idx + step)

    def _key_play(self, event):
        if not self._typing(event):
            self.toggle_play()

    # ------------------------------------------------------------ helpers
    @property
    def frame_count(self) -> int:
        return self.sti.frame_count if self.sti else 0

    @property
    def frame(self) -> sti.Frame | None:
        if not self.sti or self.sti.is_rgb or not self.sti.frames:
            return None
        return self.sti.frames[self.frame_idx]

    def _require_editable(self) -> bool:
        if not self.sti:
            return False
        if not self.sti.editable:
            messagebox.showinfo(APP_TITLE, "Ten wariant STI jest tylko do podglądu/eksportu "
                "(edycja obsługuje pliki indeksowane z kompresją ETRLE -- zob. README).")
            return False
        return True

    def _set_status(self, text):
        self.status.config(text=text)

    def _mark_dirty(self):
        self.dirty = True
        self._update_title()

    def _update_title(self):
        name = os.path.basename(self.path) if self.path else "(nowy)"
        self.root.title(f"{APP_TITLE} -- {name}{' *' if self.dirty else ''}" if self.sti else APP_TITLE)

    # --------------------------------------------------------------- files
    def _confirm_discard(self) -> bool:
        if not self.dirty:
            return True
        ans = messagebox.askyesnocancel(APP_TITLE, "Zapisać zmiany przed kontynuacją?")
        if ans is None:
            return False
        if ans:
            return self.save()
        return True

    def open_file(self, path=None):
        if not self._confirm_discard():
            return
        if path is None:
            path = filedialog.askopenfilename(filetypes=[("STI", "*.sti *.STI"), ("Wszystkie", "*.*")])
            if not path:
                return
        try:
            s = sti.STIFile.load(path)
        except (OSError, sti.STIError) as exc:
            messagebox.showerror(APP_TITLE, f"Nie można otworzyć pliku:\n{exc}")
            return
        self._set_document(s, path)

    def _set_document(self, s, path):
        self.stop_play()
        self.sti, self.path = s, path
        self.dirty = False
        self.frame_idx = 0
        self.undo_stack.clear()
        self.redo_stack.clear()
        if self.color is not None and s.is_indexed and self.color >= s.number_of_colours:
            self.color = 1
        self._fit_zoom()
        self._update_all()

    def new_from_png(self):
        if not self._confirm_discard():
            return
        path = filedialog.askopenfilename(filetypes=[("Obrazy", "*.png *.bmp *.gif"), ("Wszystkie", "*.*")])
        if not path:
            return
        try:
            s = sti.STIFile.new_from_image(Image.open(path))
        except Exception as exc:  # noqa: BLE001 -- surface any Pillow error
            messagebox.showerror(APP_TITLE, f"Nie można wczytać obrazu:\n{exc}")
            return
        self._set_document(s, None)
        self.dirty = True
        self._update_title()

    def save(self) -> bool:
        if not self.sti:
            return False
        if not self.path:
            return self.save_as()
        return self._write(self.path)

    def save_as(self) -> bool:
        if not self.sti:
            return False
        path = filedialog.asksaveasfilename(defaultextension=".sti", filetypes=[("STI", "*.sti")],
            initialfile=os.path.basename(self.path) if self.path else "nowy.sti")
        if not path:
            return False
        if self._write(path):
            self.path = path
            self._update_title()
            return True
        return False

    def _write(self, path) -> bool:
        try:
            data = self.sti.to_bytes()
            sti.STIFile.from_bytes(data)  # verify before touching the disk
            with open(path, "wb") as fh:
                fh.write(data)
        except (OSError, sti.STIError) as exc:
            messagebox.showerror(APP_TITLE, f"Zapis nie powiódł się:\n{exc}")
            return False
        self.dirty = False
        self._update_title()
        self._set_status(f"Zapisano {path} ({len(data)} B)")
        self._update_info()
        return True

    def export_frame(self):
        if not self.sti:
            return
        stem = os.path.splitext(os.path.basename(self.path or "nowy"))[0]
        path = filedialog.asksaveasfilename(defaultextension=".png", filetypes=[("PNG", "*.png")],
            initialfile=f"{stem}_{self.frame_idx:03d}.png")
        if path:
            self.sti.export_png(self.frame_idx, path)
            self._set_status(f"Wyeksportowano klatkę {self.frame_idx} -> {path}")

    def export_mask(self):
        if not self.frame:
            return
        stem = os.path.splitext(os.path.basename(self.path or "nowy"))[0]
        path = filedialog.asksaveasfilename(defaultextension=".png", filetypes=[("PNG", "*.png")],
            initialfile=f"{stem}_{self.frame_idx:03d}_mask.png")
        if path:
            self.frame.mask_image().save(path, "PNG")
            self._set_status(f"Wyeksportowano maskę klatki {self.frame_idx} -> {path}")

    def export_all(self):
        if not self.sti:
            return
        directory = filedialog.askdirectory()
        if not directory:
            return
        stem = os.path.splitext(os.path.basename(self.path or "nowy"))[0]
        paths = self.sti.export_all_png(directory, stem)
        self._set_status(f"Wyeksportowano {len(paths)} klatek do {directory}")

    def import_png(self):
        if not self._require_editable():
            return
        path = filedialog.askopenfilename(filetypes=[("Obrazy", "*.png *.bmp *.gif"), ("Wszystkie", "*.*")])
        if not path:
            return
        try:
            img = Image.open(path)
            img.load()
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror(APP_TITLE, f"Nie można wczytać obrazu:\n{exc}")
            return
        pixels, mask = sti.image_to_frame_data(img, self.sti.palette)
        self._push_frame_undo()
        self.frame.replace_pixels(pixels, mask, img.width, img.height)
        self._after_edit()
        self._update_all()

    def quit(self):
        if self._confirm_discard():
            self.root.destroy()

    # -------------------------------------------------------------- frames
    def goto_frame(self, idx):
        if not self.sti:
            return
        idx = max(0, min(self.frame_count - 1, idx))
        if idx != self.frame_idx:
            self.frame_idx = idx
            self._update_all()

    def _frame_entry(self):
        try:
            self.goto_frame(int(self.frame_var.get()))
        except ValueError:
            self.frame_var.set(str(self.frame_idx))

    def toggle_play(self):
        if self.playing:
            self.stop_play()
        elif self.sti and self.frame_count > 1:
            self.playing = True
            self.play_btn.config(text="⏸ Pauza")
            self._play_step()

    def stop_play(self):
        self.playing = False
        if self._play_job:
            self.root.after_cancel(self._play_job)
            self._play_job = None
        if hasattr(self, "play_btn"):
            self.play_btn.config(text="▶ Odtwórz")

    def _play_step(self):
        if not self.playing:
            return
        self.frame_idx = (self.frame_idx + 1) % self.frame_count
        self._update_all()
        try:
            delay = max(10, int(self.delay.get()))
        except (ValueError, tk.TclError):
            delay = 100
        self._play_job = self.root.after(delay, self._play_step)

    def duplicate_frame(self):
        if not self._require_editable():
            return
        self._push_frames_undo()
        f = self.frame.copy()
        f.mark_modified()
        self.sti.frames.insert(self.frame_idx + 1, f)
        self.frame_idx += 1
        self._after_edit()
        self._update_all()

    def new_frame(self):
        if not self._require_editable():
            return
        cur = self.frame
        w = simpledialog.askinteger(APP_TITLE, "Szerokość nowej klatki:", initialvalue=cur.width, minvalue=1, maxvalue=65535)
        if not w:
            return
        h = simpledialog.askinteger(APP_TITLE, "Wysokość nowej klatki:", initialvalue=cur.height, minvalue=1, maxvalue=65535)
        if not h:
            return
        self._push_frames_undo()
        f = sti.Frame(w, h)
        if self.sti.has_aux:
            f.aux = sti.AuxData()
        self.sti.frames.insert(self.frame_idx + 1, f)
        self.frame_idx += 1
        self._after_edit()
        self._update_all()

    def delete_frame(self):
        if not self._require_editable():
            return
        if len(self.sti.frames) <= 1:
            messagebox.showinfo(APP_TITLE, "Plik musi mieć co najmniej jedną klatkę.")
            return
        self._push_frames_undo()
        del self.sti.frames[self.frame_idx]
        self.frame_idx = min(self.frame_idx, len(self.sti.frames) - 1)
        self._after_edit()
        self._update_all()

    def clear_frame(self):
        if not self._require_editable():
            return
        self._push_frame_undo()
        f = self.frame
        f.replace_pixels(bytes(f.width * f.height), bytes(f.width * f.height))
        self._after_edit()
        self.render()

    def apply_offset(self):
        if not self._require_editable():
            return
        try:
            x, y = int(self.off_x.get()), int(self.off_y.get())
        except ValueError:
            messagebox.showerror(APP_TITLE, "Pozycja X/Y musi być liczbą całkowitą.")
            return
        if not (-32768 <= x <= 32767 and -32768 <= y <= 32767):
            messagebox.showerror(APP_TITLE, "Pozycja X/Y musi mieścić się w INT16 (-32768..32767).")
            return
        f = self.frame
        if (x, y) != (f.offset_x, f.offset_y):
            self._push_frame_undo()
            f.offset_x, f.offset_y = x, y
            self._after_edit()
            self._update_info()

    def apply_aux(self):
        if not self._require_editable() or self.frame.aux is None:
            return
        limits = {"tile_loc_index": 65535}
        new = self.frame.aux.copy()
        try:
            for key, var in self.aux_vars.items():
                v = int(var.get(), 0)
                if not 0 <= v <= limits.get(key, 255):
                    raise ValueError(key)
                setattr(new, key, v)
        except ValueError:
            messagebox.showerror(APP_TITLE, "Nieprawidłowa wartość AuxObjectData (UINT8, usTileLocIndex: UINT16).")
            return
        if new.pack() != self.frame.aux.pack():
            self._push_frame_undo()
            self.frame.aux = new
            self._after_edit()
            self._update_info()

    # --------------------------------------------------------------- undo
    def _push(self, snap):
        self.undo_stack.append(snap)
        del self.undo_stack[:-UNDO_LIMIT]
        self.redo_stack.clear()

    def _push_frame_undo(self):
        self._push(Snapshot("frame", self.frame_idx, self.frame.copy()))

    def _push_frames_undo(self):
        self._push(Snapshot("frames", data=list(self.sti.frames), current=self.frame_idx))

    def _push_palette_undo(self):
        self._push(Snapshot("palette", data=list(self.sti.palette)))

    def _capture(self, snap):
        if snap.kind == "frame":
            return Snapshot("frame", snap.index, self.sti.frames[snap.index].copy())
        if snap.kind == "frames":
            return Snapshot("frames", data=list(self.sti.frames), current=self.frame_idx)
        return Snapshot("palette", data=list(self.sti.palette))

    def _apply(self, snap):
        if snap.kind == "frame":
            self.sti.frames[snap.index].restore_from(snap.data)
            self.frame_idx = snap.index
        elif snap.kind == "frames":
            self.sti.frames[:] = snap.data
            self.frame_idx = min(snap.current, len(self.sti.frames) - 1)
        else:
            self.sti.palette[:] = snap.data

    def undo(self):
        if self.sti and self.undo_stack:
            snap = self.undo_stack.pop()
            self.redo_stack.append(self._capture(snap))
            self._apply(snap)
            self._mark_dirty()
            self._update_all()

    def redo(self):
        if self.sti and self.redo_stack:
            snap = self.redo_stack.pop()
            self.undo_stack.append(self._capture(snap))
            self._apply(snap)
            self._mark_dirty()
            self._update_all()

    def _after_edit(self):
        self._mark_dirty()

    # ------------------------------------------------------------- palette
    def select_color(self, index):
        self.color = index
        self._update_palette()

    def _palette_index_at(self, event):
        col, row = event.x // PALETTE_CELL, event.y // PALETTE_CELL
        if 0 <= col < 16 and 0 <= row < 16:
            return row * 16 + col
        return None

    def _palette_click(self, event):
        i = self._palette_index_at(event)
        if i is not None and self.sti and self.sti.is_indexed:
            self.select_color(i)

    def _palette_hover(self, event):
        i = self._palette_index_at(event)
        if i is not None and self.sti and self.sti.is_indexed:
            r, g, b = self.sti.palette[i]
            self._set_status(f"Paleta: indeks {i}  RGB({r}, {g}, {b})  #{r:02X}{g:02X}{b:02X}")

    def _palette_edit(self, event):
        i = self._palette_index_at(event)
        if i is None or not self._require_editable():
            return
        old = self.sti.palette[i]
        res = colorchooser.askcolor(color="#%02x%02x%02x" % old, title=f"Kolor palety {i}")
        if res and res[0]:
            new = tuple(int(c) for c in res[0])
            if new != old:
                self._push_palette_undo()
                self.sti.palette[i] = new
                self._after_edit()
                self.select_color(i)
                self.render()

    def import_palette(self, path=None):
        if not self._require_editable():
            return
        if path is None:
            path = filedialog.askopenfilename(title="Importuj paletę", filetypes=[
                ("Palety i obrazy", "*.pal *.act *.gpl *.sti *.png *.bmp *.gif"),
                ("Wszystkie", "*.*")])
            if not path:
                return
        try:
            new = palette_io.load_palette(path)
        except (OSError, palette_io.PaletteError) as exc:
            messagebox.showerror(APP_TITLE, f"Nie można wczytać palety:\n{exc}")
            return
        if list(self.sti.palette[:len(new)]) == new:
            self._set_status(f"Paleta z {os.path.basename(path)} jest identyczna -- brak zmian")
            return
        self._push_palette_undo()
        changed = palette_io.apply_palette(self.sti.palette, new)
        self._after_edit()
        self._update_palette()
        self.render()
        note = "" if len(new) == palette_io.PALETTE_SIZE else \
            f" (plik ma {len(new)} kolorów -- zastąpiono tylko indeksy 0..{len(new) - 1})"
        self._set_status(f"Zaimportowano paletę z {os.path.basename(path)}: zmieniono {changed} kolorów{note}")

    def export_palette(self, path=None, fmt=None):
        if not self.sti or not self.sti.is_indexed:
            return
        if path is None:
            fmt_var = tk.StringVar(value=palette_io.FORMATS["jasc"])
            stem = os.path.splitext(os.path.basename(self.path or "nowy"))[0]
            path = filedialog.asksaveasfilename(title="Eksportuj paletę", initialfile=f"{stem}.pal",
                defaultextension=".pal", typevariable=fmt_var, filetypes=[
                    (label.rsplit(" (", 1)[0], label.rsplit("(", 1)[1].rstrip(")"))
                    for label in palette_io.FORMATS.values()])
            if not path:
                return
            chosen = fmt_var.get()
            fmt = next((k for k, label in palette_io.FORMATS.items() if label.startswith(chosen)), None)
            if fmt is None or (fmt in ("jasc", "riff") and not path.lower().endswith(".pal")):
                fmt = palette_io.guess_format(path)
        try:
            used = palette_io.save_palette(path, self.sti.palette[:self.sti.number_of_colours], fmt)
        except (OSError, palette_io.PaletteError) as exc:
            messagebox.showerror(APP_TITLE, f"Nie można zapisać palety:\n{exc}")
            return
        self._set_status(f"Wyeksportowano paletę ({palette_io.FORMATS[used]}) -> {path}")

    # ------------------------------------------------------------- canvas
    def _to_pixel(self, event):
        x = int(self.canvas.canvasx(event.x) // self.zoom)
        y = int(self.canvas.canvasy(event.y) // self.zoom)
        return x, y

    def _on_motion(self, event):
        if not self.sti:
            return
        x, y = self._to_pixel(event)
        w, h, _, _ = self.sti.frame_geometry(self.frame_idx)
        if not (0 <= x < w and 0 <= y < h):
            self._set_status(f"x={x} y={y} (poza obrazem)")
            return
        if self.frame:
            idx, opaque = self.frame.get(x, y)
            r, g, b = self.sti.palette[idx]
            what = f"indeks {idx}  RGB({r}, {g}, {b})" if opaque else f"przezroczysty (indeks {idx})"
        else:
            r, g, b, a = self.sti.frame_rgba(self.frame_idx).getpixel((x, y))
            what = f"RGBA({r}, {g}, {b}, {a})"
        self._set_status(f"x={x} y={y}  {what}")

    def _on_pick(self, event):
        if not self.frame:
            return
        x, y = self._to_pixel(event)
        f = self.frame
        if 0 <= x < f.width and 0 <= y < f.height:
            idx, opaque = f.get(x, y)
            self.select_color(idx if opaque else None)

    def _paint_color(self):
        return None if self.tool.get() == "eraser" else self.color

    def _on_press(self, event):
        tool = self.tool.get()
        if tool == "picker":
            self._on_pick(event)
            return
        if not self.frame or not self._require_editable():
            return
        f = self.frame
        p = self._to_pixel(event)
        if tool in ("pencil", "eraser"):
            self._push_frame_undo()
            self._stroke = [p]
            self._paint(drawing.brush_mask(f.width, f.height, [p], self._brush_size()))
        elif tool == "fill":
            if 0 <= p[0] < f.width and 0 <= p[1] < f.height:
                self._push_frame_undo()
                self._paint(drawing.flood_fill_mask(f.pixels, f.mask, f.width, f.height, p))
        elif tool == "text":
            text = simpledialog.askstring(APP_TITLE, "Tekst:")
            if text:
                self._push_frame_undo()
                self._paint(drawing.text_mask(f.width, f.height, p, text, self._font_size()))
        else:
            self._drag_start = p

    def _on_drag(self, event):
        if not self.frame:
            return
        tool = self.tool.get()
        f = self.frame
        p = self._to_pixel(event)
        if tool in ("pencil", "eraser") and self._stroke:
            if p != self._stroke[-1]:
                mask = drawing.brush_mask(f.width, f.height, [self._stroke[-1], p], self._brush_size())
                self._stroke.append(p)
                self._paint(mask)
        elif self._drag_start is not None:
            self._preview_mask = self._shape_mask(tool, self._drag_start, p)
            self._schedule_render()

    def _on_release(self, event):
        if not self.frame:
            return
        tool = self.tool.get()
        if self._drag_start is not None:
            mask = self._shape_mask(tool, self._drag_start, self._to_pixel(event))
            self._drag_start = None
            self._preview_mask = None
            if mask is not None:
                self._push_frame_undo()
                self._paint(mask)
            self.render()
        self._stroke = []

    def _shape_mask(self, tool, p0, p1):
        f = self.frame
        if tool == "line":
            return drawing.line_mask(f.width, f.height, p0, p1, self._brush_size())
        if tool == "rect":
            return drawing.rect_mask(f.width, f.height, p0, p1, self.filled.get())
        if tool == "ellipse":
            return drawing.ellipse_mask(f.width, f.height, p0, p1, self.filled.get())
        return None

    def _paint(self, mask):
        if self.frame.paint(mask, self._paint_color()):
            self._after_edit()
        self._schedule_render()

    def _brush_size(self):
        try:
            return max(1, min(16, int(self.brush.get())))
        except (ValueError, tk.TclError):
            return 1

    def _font_size(self):
        try:
            return max(6, min(72, int(self.font_size.get())))
        except (ValueError, tk.TclError):
            return 12

    # ------------------------------------------------------------- render
    def _max_zoom(self):
        if not self.sti:
            return ZOOM_LEVELS[-1]
        w, h, _, _ = self.sti.frame_geometry(self.frame_idx)
        best = 1
        for z in ZOOM_LEVELS:
            if w * h * z * z <= MAX_RENDER_PIXELS:
                best = z
        return best

    def _fit_zoom(self):
        if not self.sti:
            return
        w, h, _, _ = self.sti.frame_geometry(0)
        avail_w, avail_h = 900, 600
        z = 1
        for level in ZOOM_LEVELS:
            if w * level <= avail_w and h * level <= avail_h:
                z = level
        self.zoom = min(z, self._max_zoom())

    def change_zoom(self, step):
        if step > 0:
            bigger = [z for z in ZOOM_LEVELS if z > self.zoom and z <= self._max_zoom()]
            if bigger:
                self.zoom = bigger[0]
        else:
            smaller = [z for z in ZOOM_LEVELS if z < self.zoom]
            if smaller:
                self.zoom = smaller[-1]
        self.render()

    def _schedule_render(self):
        if self._render_job is None:
            self._render_job = self.root.after(15, self.render)

    def _background(self, w, h):
        bg = self.background.get()
        if bg == "checker":
            tile = Image.new("RGBA", (16, 16), (200, 200, 200, 255))
            d = ImageDraw.Draw(tile)
            d.rectangle([0, 0, 7, 7], fill=(150, 150, 150, 255))
            d.rectangle([8, 8, 15, 15], fill=(150, 150, 150, 255))
            img = Image.new("RGBA", (w, h))
            for yy in range(0, h, 16):
                for xx in range(0, w, 16):
                    img.paste(tile, (xx, yy))
            return img
        colour = {"black": (0, 0, 0, 255), "magenta": (255, 0, 255, 255), "white": (255, 255, 255, 255)}[bg]
        return Image.new("RGBA", (w, h), colour)

    def render(self):
        self._render_job = None
        self.canvas.delete("all")
        if not self.sti:
            self._photo = None
            return
        self.zoom = min(self.zoom, self._max_zoom())
        self.zoom_label.config(text=f"{self.zoom}×")
        if self.show_mask.get() and self.frame:
            img = self.frame.mask_image().convert("RGBA")
        else:
            img = self.sti.frame_rgba(self.frame_idx)
            if self._preview_mask is not None and self.frame:
                overlay = Image.frombytes("L", img.size, self._preview_mask)
                c = self._paint_color()
                fill = (255, 0, 255, 160) if c is None else (*self.sti.palette[c], 255)
                img = img.copy()
                img.paste(Image.new("RGBA", img.size, fill), (0, 0), overlay)
        w, h = img.size
        z = self.zoom
        scaled = img.resize((w * z, h * z), Image.NEAREST) if z != 1 else img
        base = self._background(w * z, h * z)
        base.alpha_composite(scaled)
        if self.show_grid.get() and z >= 6:
            d = ImageDraw.Draw(base)
            for gx in range(0, w * z + 1, z):
                d.line([(gx, 0), (gx, h * z)], fill=(0, 0, 0, 90))
            for gy in range(0, h * z + 1, z):
                d.line([(0, gy), (w * z, gy)], fill=(0, 0, 0, 90))
        self._photo = ImageTk.PhotoImage(base)
        self.canvas.create_image(0, 0, anchor="nw", image=self._photo)
        self.canvas.configure(scrollregion=(0, 0, w * z, h * z))

    def _update_palette(self):
        c = self.pal_canvas
        c.delete("all")
        self.swatch.delete("all")
        if not self.sti or not self.sti.is_indexed:
            self.color_label.config(text="(brak palety -- plik RGB)" if self.sti else "")
            return
        for i, (r, g, b) in enumerate(self.sti.palette[:256]):
            x, y = (i % 16) * PALETTE_CELL, (i // 16) * PALETTE_CELL
            c.create_rectangle(x, y, x + PALETTE_CELL, y + PALETTE_CELL, fill="#%02x%02x%02x" % (r, g, b), outline="#101010")
        if self.color is not None:
            i = self.color
            x, y = (i % 16) * PALETTE_CELL, (i // 16) * PALETTE_CELL
            c.create_rectangle(x, y, x + PALETTE_CELL, y + PALETTE_CELL, outline="#ffffff", width=2)
            r, g, b = self.sti.palette[i]
            self.swatch.create_rectangle(0, 0, 40, 30, fill="#%02x%02x%02x" % (r, g, b), outline="")
            self.color_label.config(text=f"Indeks {i}  RGB({r}, {g}, {b})")
        else:
            self.swatch.create_rectangle(0, 0, 40, 30, fill="#ff00ff", outline="")
            self.swatch.create_line(0, 0, 40, 30, fill="#000000")
            self.color_label.config(text="Przezroczysty (gumka)")

    def _update_info(self):
        s = self.sti
        if not s:
            self.size_label.config(text="")
            self.file_label.config(text="Otwórz plik .sti (Plik → Otwórz…)")
            self.count_label.config(text="/ 0")
            self.frame_var.set("0")
            return
        w, h, ox, oy = s.frame_geometry(self.frame_idx)
        self.size_label.config(text=f"{w} × {h}")
        self.off_x.set(str(ox))
        self.off_y.set(str(oy))
        self.frame_var.set(str(self.frame_idx))
        self.count_label.config(text=f"/ {self.frame_count - 1}  (klatek: {self.frame_count})")
        f = self.frame
        state = "normal" if f is not None and f.aux is not None else "disabled"
        for key, var in self.aux_vars.items():
            var.set(str(getattr(f.aux, key)) if state == "normal" else "")
        for child in self.aux_frame.winfo_children():
            if isinstance(child, (ttk.Entry, ttk.Button)):
                child.configure(state=state)
        if state == "normal":
            self.aux_flags_label.config(text="fFlags: " + sti.flag_names(f.aux.flags, sti.AUX_FLAG_NAMES))
        else:
            self.aux_flags_label.config(text="Brak AuxObjectData w tym pliku.")
        info = [f"Plik: {os.path.basename(self.path) if self.path else '(nowy)'}", s.describe()]
        if f is not None and f.decode_warnings:
            info.append("klatka: " + "; ".join(f.decode_warnings[:3]))
        self.file_label.config(text="\n".join(info))

    def _update_all(self):
        self._update_title()
        self._update_info()
        self._update_palette()
        self.render()

    # ------------------------------------------------------------ analysis
    def analyze_frame(self):
        if not self.sti:
            return
        if not self.frame:
            img = self.sti.frame_rgba(0)
            messagebox.showinfo(APP_TITLE, f"Obraz RGB {img.width}×{img.height}\nbbox (alfa): {img.getbbox()}")
            return
        st = self.frame.stats(self.sti.palette)
        lines = [
            f"Klatka {self.frame_idx}: {st['size'][0]} × {st['size'][1]}, pozycja X/Y {st['offset']}",
            f"Pikseli: {st['pixels']}   nieprzezroczystych: {st['opaque']}   przezroczystych: {st['transparent']}",
            f"Obszar nieprzezroczysty (bbox x0,y0,x1,y1): {st['opaque_bbox']}",
            f"Użytych indeksów palety: {st['unique_indices']}",
            f"Nieprzezroczyste piksele o indeksie 0: {st['opaque_index0']}",
            f"Rozmiar ETRLE: {len(self.frame.encoded())} B" + (" (oryginalne bajty)" if self.frame.is_pristine else " (po edycji)"),
            "",
            "Najczęstsze indeksy (indeks: liczba, RGB):",
        ]
        lines += [f"  {i:3d}: {c:6d}  {rgb}" for i, c, rgb in st["top_indices"]]
        self._text_window(f"Analiza klatki {self.frame_idx}", "\n".join(lines))

    def file_info(self):
        if self.sti:
            self._text_window("Informacje o pliku", f"{self.path or '(nowy)'}\n\n{self.sti.describe()}")

    def _text_window(self, title, text):
        win = tk.Toplevel(self.root)
        win.title(title)
        t = tk.Text(win, width=90, height=24, font=("Consolas", 9))
        t.insert("1.0", text)
        t.config(state="disabled")
        t.pack(fill="both", expand=True)


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    root = tk.Tk()
    app = STIEditor(root)
    if argv:
        app.open_file(argv[0])
    root.mainloop()


if __name__ == "__main__":
    main()
