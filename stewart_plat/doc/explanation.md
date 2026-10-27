スチュアートプラットフォーム（6自由度並列リンク機構）のPython実装では、主に**逆運動学（IK）**と**3D可視化**が必要です。順運動学は解析解が難しく数値解法が必要なので、基本的には逆運動学から実装します。

以下に、Pythonでの基本的な実装例を示します。`numpy`で幾何計算を行い、`matplotlib`で3D描画します。

```python
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from mpl_toolplots.mplot3d.art3d import Line3DCollection
import math

class StewartPlatform:
    def __init__(self,
                 base_radius=5.0,
                 platform_radius=3.0,
                 base_half_angle=15.0,      # 度
                 platform_half_angle=10.0,  # 度
                 min_leg_length=7.0,
                 max_leg_length=12.0):
        """
        スチュアートプラットフォームの初期化
        6脚は上下それぞれ 短-長-短-長-短-長 の間隔で配置される
        """
        self.base_radius = base_radius
        self.platform_radius = platform_radius
        self.base_half_angle = np.radians(base_half_angle)
        self.platform_half_angle = np.radians(platform_half_angle)
        self.min_leg_length = min_leg_length
        self.max_leg_length = max_leg_length
        
        # ベース（下側）の接続点を計算
        self.base_points = self._calculate_joint_points(
            base_radius, base_half_angle, is_base=True
        )
        
        # プラットフォーム（上側）の接続点を計算
        self.platform_points_home = self._calculate_joint_points(
            platform_radius, platform_half_angle, is_base=False
        )
        
        # ホーム位置（中立姿勢）での脚長
        self.home_lengths = self._calculate_home_leg_lengths()
        
    def _calculate_joint_points(self, radius, half_angle, is_base):
        """
        6つの接続点を生成
        各ペアは ±half_angle の位置に配置
        """
        points = []
        for i in range(6):
            angle = (i * 60.0) * np.pi / 180.0
            if is_base:
                # ベース: 偶数脚が -half, 奇数脚が +half
                offset = -half_angle if i % 2 == 0 else half_angle
            else:
                # プラットフォーム: 偶数脚が +half, 奇数脚が -half（交互に配置）
                offset = half_angle if i % 2 == 0 else -half_angle
            
            theta = angle + offset
            x = radius * np.cos(theta)
            y = radius * np.sin(theta)
            z = 0.0
            points.append(np.array([x, y, z]))
        return np.array(points)
    
    def _calculate_home_leg_lengths(self):
        """中立姿勢での脚長を計算"""
        lengths = []
        for i in range(6):
            leg = self.platform_points_home[i] - self.base_points[i]
            lengths.append(np.linalg.norm(leg))
        return np.array(lengths)
    
    def _rotation_matrix(self, roll, pitch, yaw):
        """
        回転行列を生成（XYZ固定角、すなわちロール-ピッチ-ヨー）
        単位: ラジアン
        """
        # ロール (X軸回り)
        Rx = np.array([
            [1, 0, 0],
            [0, np.cos(roll), -np.sin(roll)],
            [0, np.sin(roll), np.cos(roll)]
        ])
        # ピッチ (Y軸回り)
        Ry = np.array([
            [np.cos(pitch), 0, np.sin(pitch)],
            [0, 1, 0],
            [-np.sin(pitch), 0, np.cos(pitch)]
        ])
        # ヨー (Z軸回り)
        Rz = np.array([
            [np.cos(yaw), -np.sin(yaw), 0],
            [np.sin(yaw), np.cos(yaw), 0],
            [0, 0, 1]
        ])
        return Rz @ Ry @ Rx
    
    def inverse_kinematics(self, position, orientation):
        """
        逆運動学: プラットフォームの姿勢から6本の脚長を計算
        
        Parameters:
            position: [x, y, z] プラットフォーム中心の位置
            orientation: [roll, pitch, yaw] 単位: ラジアン
        
        Returns:
            leg_lengths: 6本の脚長
            platform_points: 各脚の上端接続点（世界座標系）
        """
        position = np.array(position)
        roll, pitch, yaw = orientation
        
        # 回転行列
        R = self._rotation_matrix(roll, pitch, yaw)
        
        # プラットフォーム接続点を世界座標系に変換
        platform_points = []
        for p_local in self.platform_points_home:
            p_world = position + R @ p_local
            platform_points.append(p_world)
        platform_points = np.array(platform_points)
        
        # 各脚の長さを計算
        leg_lengths = []
        for i in range(6):
            leg_vector = platform_points[i] - self.base_points[i]
            length = np.linalg.norm(leg_vector)
            leg_lengths.append(length)
            
            # 可動範囲チェック
            if length < self.min_leg_length or length > self.max_leg_length:
                print(f"警告: 脚{i}が可動範囲外: {length:.3f}")
        
        return np.array(leg_lengths), platform_points
    
    def plot(self, position, orientation, ax=None):
        """
        3Dプロット
        """
        if ax is None:
            fig = plt.figure(figsize=(10, 8))
            ax = fig.add_subplot(111, projection='3d')
        
        leg_lengths, platform_points = self.inverse_kinematics(position, orientation)
        
        # ベースをプロット（青）
        base_loop = np.vstack([self.base_points, self.base_points[0]])
        ax.plot(base_loop[:, 0], base_loop[:, 1], base_loop[:, 2], 'b-', linewidth=2)
        ax.scatter(self.base_points[:, 0], self.base_points[:, 1], self.base_points[:, 2], c='blue', s=50)
        
        # プラットフォームをプロット（赤）
        plat_loop = np.vstack([platform_points, platform_points[0]])
        ax.plot(plat_loop[:, 0], plat_loop[:, 1], plat_loop[:, 2], 'r-', linewidth=2)
        ax.scatter(platform_points[:, 0], platform_points[:, 1], platform_points[:, 2], c='red', s=50)
        
        # 脚をプロット（緑）
        for i in range(6):
            x = [self.base_points[i, 0], platform_points[i, 0]]
            y = [self.base_points[i, 1], platform_points[i, 1]]
            z = [self.base_points[i, 2], platform_points[i, 2]]
            ax.plot(x, y, z, 'g-', linewidth=1.5)
        
        # プラットフォーム中心
        ax.scatter(position[0], position[1], position[2], c='black', s=100, marker='*')
        
        # 軸設定
        max_range = self.max_leg_length * 0.8
        ax.set_xlim([-max_range, max_range])
        ax.set_ylim([-max_range, max_range])
        ax.set_zlim([0, max_range * 1.5])
        ax.set_xlabel('X')
        ax.set_ylabel('Y')
        ax.set_zlabel('Z')
        ax.set_title(f'Stewart Platform\nPos: {position}, RPY: {np.degrees(orientation)}')
        
        return leg_lengths


# 使用例
if __name__ == "__main__":
    # プラットフォーム生成
    platform = StewartPlatform(
        base_radius=5.0,
        platform_radius=3.0,
        base_half_angle=15.0,
        platform_half_angle=10.0,
        min_leg_length=5.0,
        max_leg_length=15.0
    )
    
    # 中立姿勢（ホームポジション）
    home_z = np.mean(platform.home_lengths)
    position = [0, 0, home_z * 0.9]  # 少し下げる
    orientation = [0, 0, 0]  # ラジアン
    
    # プロット
    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')
    
    # 1. 中立姿勢
    lengths = platform.plot(position, orientation, ax)
    print("中立姿勢の脚長:", lengths)
    
    plt.tight_layout()
    plt.show()
    
    # 2. 傾けた姿勢の例
    fig, axes = plt.subplots(1, 2, figsize=(14, 6), subplot_kw={'projection': '3d'})
    
    # ピッチ 10度
    pos2 = [0, 0, home_z * 0.9]
    ori2 = [np.radians(0), np.radians(10), np.radians(0)]
    lengths2 = platform.plot(pos2, ori2, axes[0])
    print("ピッチ10度の脚長:", lengths2)
    
    # ロール 10度 + Z移動
    pos3 = [0.5, 0, home_z * 0.85]
    ori3 = [np.radians(10), np.radians(5), np.radians(0)]
    lengths3 = platform.plot(pos3, ori3, axes[1])
    print("複合姿勢の脚長:", lengths3)
    
    plt.tight_layout()
    plt.show()
```

## 実装に必要な主要要素

### 1. 幾何モデリング
- **ベース・プラットフォームの接続点配置**: 6脚は通常、60度間隔で交互に配置（短-長-短-長のペア）
- **ホーム位置の設計**: 中立姿勢での脚長を基準に可動範囲を決定

### 2. 逆運動学（IK）
最も重要な部分です。プラットフォームの姿勢（位置 + 回転）から各脚の長さを計算します。

```
leg_vector = platform_position + R * platform_joint_local - base_joint
leg_length = |leg_vector|
```

`R` はロール・ピッチ・ヨーから作る回転行列です。

### 3. 可動範囲チェック
各脚の計算された長さが、シリンダの伸縮範囲内に収まっているか検証する必要があります。

### 4. 順運動学（FK）（オプション・発展）
与えられた6本の脚長からプラットフォームの姿勢を求める問題です。解析解は存在せず、**Newton-Raphson法**などの数値解法で解きます。制御用途では通常IKのみで十分です。

### 5. ヤコビアン（速度制御・特異点解析）
脚の伸縮速度とプラットフォームの6自由度速度の関係を表します。特異点（特定の姿勢で自由度が失われる状態）の解析にも使います。

---

もし実際のハードウェア制御（ArduinoやPLCとの連携）を考えている場合は、上記のIK計算結果を各アクチュエータの目標長さとしてシリアル通信やMQTTで送信する層が追加で必要です。特定の用途（シミュレーション、実機制御、軌道生成など）に応じてさらに詳しく説明できます。


実際のハードウェアでスチュアートプラットフォームを構築する場合、**メカニカル・制御・ソフトウェア・安全**の4層で設計するのが一般的です。用途（モーションシミュレータ、精密定位台、振動台など）によって規模は変わりますが、構成の基本は共通しています。

---

## 1. メカニカル構成

### アクチュエータ
最も一般的なのは**電動シリンダ（ボールねじ or ローラーねじ）**です。

| 方式 | 特徴 | 用途 |
|------|------|------|
| **ボールねじシリンダ** | 安価、効率が高い、バックラッシュあり | 中精度、モーションシミュレータ |
| **ローラーねじシリンダ** | 高剛性、高精度、長寿命、高価 | 精密定位、計測用ステージ |
| **油圧シリンダ** | 非常に高推力、高速、メンテが必要 | 大型フライトシミュレータ、重載用途 |

### ジョイント（両端の接続部）
上下両端に**球面軸受（ロッドエンド）**を使用します。2軸または3軸のヒンジでは自由度が拘束され、機構がロックするため、6脚すべての両端に球面軸受が必須です。可動域は通常 ±25度〜±35度程度を確保します。

### フレーム（ベース・ムーバー）
- **ベース側**: 厚い鋼板またはアルミフレーム（例: 4040アルミ型材）で剛性確保
- **ムーバー側（可動部）**: 軽量化のためアルミ板やカーボンを使用。重心をできるだけ低くする設計が望ましいです。

---

## 2. 制御系構成

### コントローラ
用途に応じて選択します。

| 規模 | 推奨構成 |
|------|----------|
| **小型・実験用** | Arduino Mega / ESP32 / Raspberry Pi Pico + モータドライバ |
| **中型・産業用** | LinuxCNC / Mach4 / 専用CNCコントローラ |
| **高精度・リアルタイム** | **TwinCAT（Beckhoff）** / **National Instruments（CompactRIO）** / **QNXリアルタイムLinux** |

6軸を同時に補間制御する必要があるため、**リアルタイム性の高いPLCまたはPCベースのモーションコントローラ**が推奨されます。

### モータとドライバ
- **モータ**: ACサーボモータ（三菱、安川、Panasonicなど）が最も一般的。安価にするならステッピングモータも可能ですが、トルク不足や失調に注意が必要です。
- **ドライバ**: 各軸独立のサーボアンプ。位置・速度・トルクの3ループ制御を内蔵しているものが望ましいです。
- **フィードバック**: モータにエンコーダ（インクリメンタルまたはアブソリュート）を搭載。さらに精度が必要なら脚に外部リニアエンコーダを追加します。

### センサ類
- **リミットスイッチ**: 各脚のストローク端（上下）に設置し、オーバートラベル防止
- **原点センサ**: ホームポジション検出用
- **IMU（慣性計測装置）**: ムーバー側に取り付け、実際の姿勢をフィードバック（クローズドループ化）
- **ロードセル**: 各脚の推力計測（オプション）

---

## 3. ソフトウェア構成

### 制御ループの流れ
```
目標姿勢 (X, Y, Z, Roll, Pitch, Yaw)
    ↓
逆運動学（IK）計算 → 6本の目標脚長
    ↓
各軸の位置指令値（パルス or アナログ電圧）
    ↓
サーボドライバ → モータ駆動
    ↓
エンコーダフィードバック → 実際の脚長を確認
```

### 推奨ソフトウェア構成
- **上位PC**: Python / C++ でIK・軌道生成（S字加減速、直線補間）を実装
- **通信**: EtherCAT（リアルタイム性が高い）、CANopen、Modbus、または単純なパルス列
- **下位（組み込み）**: リアルタイムOS上で1kHz〜10kHzの制御ループを回す

---

## 4. 安全系（絶対に省けない要素）

- **緊急停止（E-Stop）回路**: ハードウェア的なリレー回路で、緊急時はモータの電源を即座に遮断。ソフトウェア停止だけでは危険です。
- **ソフトリミット**: ソフトウェアで脚長の可動範囲を制限
- **ハードリミット**: リミットスイッチによる物理的な停止
- **特異点回避**: 姿勢指令値を事前に検証し、脚が水平に近づく特異姿勢に入らないようにする
- **相互干涉チェック**: 脚同士が接触しないよう、可動域をモデリングしておく

---

## 5. 構成例（中規模モーションシミュレータ想定）

| 部位 | 部品例 |
|------|--------|
| アクチュエータ | ボールねじ電動シリンダ ×6（ストローク150mm程度） |
| モータ | 400W ACサーボ ×6 |
| ドライバ | サーボアンプ ×6 |
| コントローラ | Beckhoff IPC + TwinCAT3（EtherCAT通信） |
| ジョイント | ロッドエンドボールジョイント M12〜M16 |
| フレーム | S45Cベース + アルミムーバー |
| センサ | リミットスイッチ、ムーバー側IMU |
| 電源 | 三相200V → サーボアンプ用DC電源 |
| 安全 | ハードE-Stop回路、ソフトリミット |

---

## まとめ

実機化のポイントは以下の3つです。

1. **アクチュエータは電動シリンダ（ボールねじ）+ ACサーボが標準的でバランスが良い**
2. **コントローラは6軸同時補間ができ、リアルタイム性のあるもの（TwinCATやLinuxCNC）を選ぶ**
3. **安全系はハードウェア的なE-Stopとリミットスイッチを必ず設ける**

用途や予算に応じて、小型実験機ならArduino + ステッピングモータ + 安価な電動シリンダ（DIY級）から始めることも可能です。具体的な用途（シミュレータ、定位台、試験機など）が決まっていれば、さらに詳細な選定ができます。