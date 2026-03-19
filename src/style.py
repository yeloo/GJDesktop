# -*- coding: utf-8 -*-
"""
CCdesk UI 样式定义 —— 深色现代主题
"""

# ── 调色板 ──────────────────────────────────
COLORS = {
    "bg_dark":      "#1a1d23",   # 主背景
    "bg_panel":     "#21252e",   # 面板背景
    "bg_card":      "#2a2f3a",   # 卡片背景
    "bg_hover":     "#31374a",   # 悬停背景
    "bg_selected":  "#1e3a5f",   # 选中背景
    "accent":       "#4a9eff",   # 主题蓝
    "accent_hover": "#6ab3ff",   # 主题蓝高亮
    "accent_green": "#4caf50",   # 成功绿
    "accent_red":   "#f44336",   # 危险红
    "accent_orange":"#ff9800",   # 警告橙
    "text_primary": "#e8eaf0",   # 主文字
    "text_secondary":"#8892a4",  # 次要文字
    "text_disabled": "#4a5568",  # 禁用文字
    "border":       "#2d3340",   # 边框
    "border_light": "#3d4455",   # 亮边框
    "scrollbar":    "#3d4455",   # 滚动条
    "sidebar_w":    220,         # 侧边栏宽度
}

# ── 字体 ─────────────────────────────────────
FONTS = {
    "title":    ("Microsoft YaHei UI", 14, "bold"),
    "heading":  ("Microsoft YaHei UI", 11, "bold"),
    "body":     ("Microsoft YaHei UI", 10),
    "small":    ("Microsoft YaHei UI", 9),
    "mono":     ("Consolas", 10),
    "icon":     ("Segoe UI Emoji", 14),
    "big_num":  ("Microsoft YaHei UI", 22, "bold"),
}

# ── 全局 QSS ─────────────────────────────────
APP_STYLE = f"""
/* ─── 全局 ─────────────────────── */
QWidget {{
    background-color: {COLORS['bg_dark']};
    color: {COLORS['text_primary']};
    font-family: "Microsoft YaHei UI";
    font-size: 10pt;
    outline: none;
}}
QMainWindow {{
    background-color: {COLORS['bg_dark']};
}}

/* ─── 滚动条 ────────────────────── */
QScrollBar:vertical {{
    background: transparent;
    width: 6px;
    margin: 0;
}}
QScrollBar::handle:vertical {{
    background: {COLORS['scrollbar']};
    border-radius: 3px;
    min-height: 30px;
}}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {{
    height: 0;
}}
QScrollBar:horizontal {{
    background: transparent;
    height: 6px;
}}
QScrollBar::handle:horizontal {{
    background: {COLORS['scrollbar']};
    border-radius: 3px;
}}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {{
    width: 0;
}}

/* ─── 按钮 ──────────────────────── */
QPushButton {{
    background-color: {COLORS['bg_card']};
    color: {COLORS['text_primary']};
    border: 1px solid {COLORS['border_light']};
    border-radius: 6px;
    padding: 6px 18px;
    font-size: 10pt;
}}
QPushButton:hover {{
    background-color: {COLORS['bg_hover']};
    border-color: {COLORS['accent']};
    color: {COLORS['accent_hover']};
}}
QPushButton:pressed {{
    background-color: {COLORS['bg_selected']};
}}
QPushButton:disabled {{
    background-color: {COLORS['bg_panel']};
    color: {COLORS['text_disabled']};
    border-color: {COLORS['border']};
}}
QPushButton#primary {{
    background-color: {COLORS['accent']};
    color: white;
    border: none;
    font-weight: bold;
}}
QPushButton#primary:hover {{
    background-color: {COLORS['accent_hover']};
}}
QPushButton#danger {{
    background-color: {COLORS['accent_red']};
    color: white;
    border: none;
}}
QPushButton#success {{
    background-color: {COLORS['accent_green']};
    color: white;
    border: none;
}}

/* ─── 输入框 ────────────────────── */
QLineEdit, QTextEdit, QPlainTextEdit {{
    background-color: {COLORS['bg_card']};
    color: {COLORS['text_primary']};
    border: 1px solid {COLORS['border_light']};
    border-radius: 6px;
    padding: 5px 10px;
    selection-background-color: {COLORS['accent']};
}}
QLineEdit:focus, QTextEdit:focus {{
    border-color: {COLORS['accent']};
}}

/* ─── 下拉框 ────────────────────── */
QComboBox {{
    background-color: {COLORS['bg_card']};
    color: {COLORS['text_primary']};
    border: 1px solid {COLORS['border_light']};
    border-radius: 6px;
    padding: 5px 10px;
    min-width: 120px;
}}
QComboBox:hover {{
    border-color: {COLORS['accent']};
}}
QComboBox::drop-down {{
    border: none;
    width: 24px;
}}
QComboBox::down-arrow {{
    image: none;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid {COLORS['text_secondary']};
    margin-right: 6px;
}}
QComboBox QAbstractItemView {{
    background-color: {COLORS['bg_card']};
    color: {COLORS['text_primary']};
    border: 1px solid {COLORS['border_light']};
    selection-background-color: {COLORS['accent']};
    outline: none;
}}

/* ─── 列表 / 表格 ────────────────── */
QListWidget, QTreeWidget, QTableWidget {{
    background-color: {COLORS['bg_panel']};
    color: {COLORS['text_primary']};
    border: 1px solid {COLORS['border']};
    border-radius: 8px;
    gridline-color: {COLORS['border']};
    alternate-background-color: {COLORS['bg_card']};
}}
QListWidget::item, QTreeWidget::item, QTableWidget::item {{
    padding: 4px 8px;
    border-radius: 4px;
}}
QListWidget::item:hover, QTreeWidget::item:hover, QTableWidget::item:hover {{
    background-color: {COLORS['bg_hover']};
}}
QListWidget::item:selected, QTreeWidget::item:selected, QTableWidget::item:selected {{
    background-color: {COLORS['bg_selected']};
    color: {COLORS['accent_hover']};
}}
QHeaderView::section {{
    background-color: {COLORS['bg_card']};
    color: {COLORS['text_secondary']};
    border: none;
    border-bottom: 1px solid {COLORS['border_light']};
    padding: 6px 8px;
    font-weight: bold;
}}

/* ─── 进度条 ─────────────────────── */
QProgressBar {{
    background-color: {COLORS['bg_card']};
    border: none;
    border-radius: 4px;
    height: 8px;
    text-align: center;
    color: transparent;
}}
QProgressBar::chunk {{
    background-color: {COLORS['accent']};
    border-radius: 4px;
}}

/* ─── 标签页 ─────────────────────── */
QTabWidget::pane {{
    border: 1px solid {COLORS['border']};
    border-radius: 8px;
    background-color: {COLORS['bg_panel']};
    top: -1px;
}}
QTabBar::tab {{
    background-color: {COLORS['bg_dark']};
    color: {COLORS['text_secondary']};
    padding: 8px 20px;
    border: 1px solid {COLORS['border']};
    border-bottom: none;
    border-radius: 6px 6px 0 0;
    margin-right: 2px;
}}
QTabBar::tab:selected {{
    background-color: {COLORS['bg_panel']};
    color: {COLORS['accent']};
    border-color: {COLORS['border_light']};
}}
QTabBar::tab:hover {{
    background-color: {COLORS['bg_hover']};
    color: {COLORS['text_primary']};
}}

/* ─── 复选框 / 单选框 ─────────────── */
QCheckBox, QRadioButton {{
    color: {COLORS['text_primary']};
    spacing: 8px;
}}
QCheckBox::indicator, QRadioButton::indicator {{
    width: 16px;
    height: 16px;
    border: 2px solid {COLORS['border_light']};
    border-radius: 4px;
    background-color: {COLORS['bg_card']};
}}
QCheckBox::indicator:checked, QRadioButton::indicator:checked {{
    background-color: {COLORS['accent']};
    border-color: {COLORS['accent']};
}}
QRadioButton::indicator {{
    border-radius: 8px;
}}

/* ─── 滑动条 / 数字输入 ───────────── */
QSpinBox, QDoubleSpinBox {{
    background-color: {COLORS['bg_card']};
    color: {COLORS['text_primary']};
    border: 1px solid {COLORS['border_light']};
    border-radius: 6px;
    padding: 4px 8px;
}}
QSpinBox:focus, QDoubleSpinBox:focus {{
    border-color: {COLORS['accent']};
}}
QSpinBox::up-button, QSpinBox::down-button,
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {{
    background-color: {COLORS['bg_hover']};
    border: none;
    width: 18px;
    border-radius: 3px;
}}

/* ─── 工具提示 ────────────────────── */
QToolTip {{
    background-color: {COLORS['bg_card']};
    color: {COLORS['text_primary']};
    border: 1px solid {COLORS['border_light']};
    border-radius: 4px;
    padding: 4px 8px;
}}

/* ─── 分隔线 ─────────────────────── */
QFrame[frameShape="4"], QFrame[frameShape="5"] {{
    color: {COLORS['border']};
}}

/* ─── 消息弹框 ────────────────────── */
QMessageBox {{
    background-color: {COLORS['bg_panel']};
}}
QMessageBox QLabel {{
    color: {COLORS['text_primary']};
    font-size: 10pt;
}}

/* ─── 对话框 ─────────────────────── */
QDialog {{
    background-color: {COLORS['bg_panel']};
}}
"""
