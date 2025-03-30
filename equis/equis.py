import tkinter as tk
from tkinter import messagebox
import random
from PIL import Image, ImageTk
import pygame
import threading
import time

class HorseRacingGame:
    def __init__(self):
        self.horses = ["クラウドナイト", "ダンディオン", "ルシフェルウィング",
                      "アレスフレア", "レオンハート", "ゼウスブレイド"]
        self.horse_images = [f"{i}.png" for i in range(1, 7)]
        self.girl_images = ["7.png"]
        self.prize_distribution = [200000000, 100000000, 100000000, 0, 0, 0]
        self.horse_contributions = [0] * len(self.horses)
        self.previous_results = [0] * len(self.horses)
        self.skip_confirmation = False

        self.setup_window()
        self.setup_audio()
        self.create_ui_elements()
        self.initial_race()

    def setup_window(self):
        self.root = tk.Tk()
        self.root.title("最大３億円が当たる！イケメン競馬ゲーム「osiuma」")
        self.root.resizable(False, False)
        self.root.geometry("1250x690")

        self.canvas = tk.Canvas(self.root)
        self.canvas.pack(fill=tk.BOTH, expand=True)

        self.setup_background()

    def setup_background(self):
        bg_image = Image.open("0.png")
        new_height = self.root.winfo_screenheight()
        new_width = int(new_height * (462 / 334))
        self.bg_photo = ImageTk.PhotoImage(bg_image.resize((new_width, new_height), Image.LANCZOS))

        self.bg_x1, self.bg_x2, self.bg_x3 = 0, new_width, new_width * 2
        for x, tag in [(self.bg_x1, "background1"), (self.bg_x2, "background2"),
                      (self.bg_x3, "background3")]:
            self.canvas.create_image(x, self.root.winfo_screenheight() // 2,
                                   image=self.bg_photo, tags=tag, anchor="w")

    def setup_audio(self):
        try:
            pygame.mixer.pre_init(44100, -16, 2, 2048)
            pygame.init()
            pygame.mixer.init()
        except pygame.error:
            messagebox.showerror("エラー", "オーディオデバイスが見つかりません。"
                               "イヤホンやスピーカーを接続してください。")

    def create_ui_elements(self):
        self.create_labels()
        self.create_horse_images()
        self.create_girl_image()
        self.create_buttons()

    def create_labels(self):
        tk.Label(self.root, text="最大３億円が当たる！イケメン競馬ゲーム",
                font=("Ricty Diminished", 22)).place(x=50, y=30)

        self.horse_labels = []
        for i, horse in enumerate(self.horses):
            label = tk.Label(self.root, text=f"{horse}: {self.format_money(0)}",
                           font=("Ricty Diminished", 20))
            label.place(x=50, y=120 + 50 * i)
            self.horse_labels.append(label)

        self.result_label = tk.Label(self.root, text="", font=("Ricty Diminished", 20))
        self.result_label.place(x=40, y=520)
        
        self.total_contribution_label = tk.Label(self.root, text="", font=("Ricty Diminished", 20))
        self.total_contribution_label.place(x=800, y=520)

    def create_horse_images(self):
        self.image_canvases = []
        for i, image_file in enumerate(self.horse_images):
            img = Image.open(image_file).resize((200, 200), Image.LANCZOS)
            photo = ImageTk.PhotoImage(img)
            canvas = tk.Canvas(self.root, width=200, height=200)
            canvas.create_image(100, 100, image=photo)
            canvas.image = photo
            canvas.place(x=550 + (i % 3) * 220, y=90 + (i // 3) * 210)
            canvas.bind("<Button-1>",
                       lambda e, horse=self.horses[i]: self.show_contribution_dialog(horse))

            horse_label = tk.Label(canvas, text=self.horses[i],
                                 font=("Ricty Diminished", 14))
            horse_label.place(x=100, y=180, anchor="center")
            self.image_canvases.append(canvas)

    def create_girl_image(self):
        img = Image.open(self.girl_images[0]).resize((150, 150), Image.LANCZOS)
        photo = ImageTk.PhotoImage(img)
        canvas = tk.Canvas(self.root, width=150, height=150)
        canvas.create_image(75, 75, image=photo)
        canvas.image = photo
        canvas.place(x=1020, y=510)
        canvas.bind("<Button-1>",
                   lambda e: messagebox.showinfo("推し方", "推し方で勝負が決まるよ！"))

    def create_buttons(self):
        tk.Button(self.root, text="START", command=self.start_game,
                 font=("Ricty Diminished", 20)).place(x=380, y=430)

    def format_money(self, amount):
        if amount >= 100000000:
            oku = amount // 100000000
            man = (amount % 100000000) // 10000
            if man == 0:
                return f"{oku}億円"
            else:
                return f"{oku}億{man}万円"

        elif amount >= 10000:
            return f"{amount // 10000}万円"
        return f"{amount}円"

    def scroll_background(self):
        while pygame.mixer.music.get_busy():
            # 背景画像の座標を更新
            self.bg_x1 -= 2
            self.bg_x2 -= 2
            self.bg_x3 -= 2

            # 画面左端に到達した背景を右側に再配置
            if self.bg_x1 <= -self.bg_photo.width():
                self.bg_x1 = self.bg_x3 + self.bg_photo.width()
            if self.bg_x2 <= -self.bg_photo.width():
                self.bg_x2 = self.bg_x1 + self.bg_photo.width()
            if self.bg_x3 <= -self.bg_photo.width():
                self.bg_x3 = self.bg_x2 + self.bg_photo.width()

            # 背景画像の座標を更新
            self.canvas.coords("background1", self.bg_x1, self.root.winfo_screenheight() // 2)
            self.canvas.coords("background2", self.bg_x2, self.root.winfo_screenheight() // 2)
            self.canvas.coords("background3", self.bg_x3, self.root.winfo_screenheight() // 2)

            # 画面を更新
            self.root.update()
            time.sleep(0.01)  # スムーズなスクロールのための遅延

    def start_game(self):
        if not pygame.mixer.get_init():
            self.setup_audio()

        # すでにレースが進行中ならアラートを表示
        if pygame.mixer.music.get_busy():
            messagebox.showwarning("警告", "レース中です！途中で止めると無効になります。")
            return

        messagebox.showinfo("レース開始", "レースが始まります！最後まで推しを信じて貢ぎましょう！")

        # BGMを再生
        pygame.mixer.music.load("race_bgm.mp3")
        pygame.mixer.music.play(0)

        # 背景スクロールとユーザーシミュレーションを並行して実行
        threading.Thread(target=self.scroll_background, daemon=True).start()
        threading.Thread(target=self.simulate_users, daemon=True).start()

        # レース結果更新
        self.update_race_results()

        # 音楽終了後に結果を表示するスレッドを開始
        threading.Thread(target=self.wait_for_music_end, daemon=True).start()

    def wait_for_music_end(self):
        while pygame.mixer.music.get_busy():
            time.sleep(0.1)  # 1秒ごとにチェック

        # 音楽が終了したら賞金を計算
        self.calculate_prize()


    def calculate_prize(self):
        # 貢ぎ金が多い順にソート
        sorted_horses = sorted(zip(self.horses, self.horse_contributions), key=lambda x: x[1], reverse=True)

        # 上位3着の合計貢ぎ額
        total_money = sum(horse[1] for horse in sorted_horses[:3])

        # 3億円の上限を適用
        prize_money = min(total_money, 300000000)
        
        return total_money

    def update_race_results(self):
        race_result = sorted(self.horses,
                           key=lambda x: self.horse_contributions[self.horses.index(x)],
                           reverse=True)

        prize_money_list = [0] * len(self.horses)
        for i in range(6):
            prize_money_list[self.horses.index(race_result[i])] = self.prize_distribution[i]

        self.previous_results = prize_money_list
        
        total_contribution = self.calculate_prize()

        result_text = "予想：\n" + "  ".join(
            f"{i+1}着: {horse} (貢ぎ金額: {self.format_money(self.horse_contributions[self.horses.index(horse)])})"
            + ("\n" if i % 2 == 1 and i != 0 else "")
            for i, horse in enumerate(race_result[:6])
        )

        self.result_label.config(text=result_text, fg="blue", justify="left", anchor="w")
        self.total_contribution_label.config(text=f"合計貢ぎ金額：{self.format_money(total_contribution)}", fg="blue", justify="left", anchor="w")

    def contribute(self, horse_name):
        horse_index = self.horses.index(horse_name)
        self.horse_contributions[horse_index] += 10000000
        self.update_horse_labels()
        self.root.update()

    def show_contribution_dialog(self, horse_name):
        if self.skip_confirmation:
            self.contribute(horse_name)
            return

        dialog = tk.Toplevel(self.root)
        dialog.title("推し")

        tk.Label(dialog, text=f"{horse_name}を推しますか？(1000万円)",
                font=("Ricty Diminished", 14)).pack(pady=10)

        check_var = tk.BooleanVar()
        tk.Checkbutton(dialog, text="次回以降この確認を省略する",
                      variable=check_var).pack()

        def on_confirm():
            self.skip_confirmation = check_var.get()
            dialog.destroy()
            self.contribute(horse_name)

        button_frame = tk.Frame(dialog)
        button_frame.pack(pady=10)
        tk.Button(button_frame, text="推す", command=on_confirm).pack(side="left", padx=5)
        tk.Button(button_frame, text="キャンセル",
                 command=dialog.destroy).pack(side="right", padx=5)

        dialog.transient(self.root)
        dialog.grab_set()
        self.root.wait_window(dialog)

    def update_horse_labels(self):
        sorted_horses = sorted(zip(self.horses, self.horse_contributions,
                                 self.horse_images, self.image_canvases),
                             key=lambda x: x[1], reverse=True)
        for i, (horse_name, contribution, _, img_canvas) in enumerate(sorted_horses):
            self.horse_labels[i].config(text=f"{horse_name}: {self.format_money(contribution)}")
            img_canvas.place(x=550 + (i % 3) * 220, y=90 + (i // 3) * 210)

    def simulate_users(self):
        n = 10
        while pygame.mixer.music.get_busy():
            time.sleep(n)
            horse_index = random.randint(0, len(self.horses) - 1)
            self.horse_contributions[horse_index] += 10000000
            self.update_horse_labels()
            n = max(1, n - 1)

    def initial_race(self):
        race_result = sorted(self.horses,
                           key=lambda x: self.horse_contributions[self.horses.index(x)],
                           reverse=True)
        self.previous_results = [self.horse_contributions[self.horses.index(horse)]
                               for horse in race_result]

    def run(self):
        self.root.mainloop()

if __name__ == "__main__":
    game = HorseRacingGame()
    game.run()
