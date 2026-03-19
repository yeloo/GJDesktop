# -*- coding: utf-8 -*-
"""
CCdesk 自定义 PyQt5 控件
"""

from PyQt5.QtWidgets import (
    QWidget, QLabel, QPushButton, QHBoxLayout, QVBoxLayout,
    QFrame, QGraphicsDropShadowEffect, QSizePolicy
)
from PyQt5.QtCore import Qt, QPropertyAnimation, QRect, pyqtSignal, QPoint
from PyQt5.QtGui import QColor, QFont, QPainter, QBrush, QPen, QLinearGradient

from src.style import COLORS, FONTS


class ShadowWidget(QWidget):
    """带阴影的卡片容器"""
    def __init__(self, parent=None, radius=12):
        super().__init__(parent)
        self.setObjectName("shadowCard")
        shadow = QGraphicsDropShadowEffect(self)
        shadow.setBlurRadius(24)
        shadow.setColor(QColor(0, 0, 0, 80))
        shadow.setOffset(0, 4)
        self.setGraphicsEffect(shadow)
        self.setStyleSheet(f"""
            #shadowCard {{
                background-color: {COLORS['bg_card']};
                border-radius: {radius}px;
                border: 1px solid {COLORS['border']};
            }}
        """)


class StatCard(QWidget):
    """首页统计卡片：显示图标 + 数字 + 标签"""
    def __init__(self, icon: str, label: str, value: str,
                 color: str = COLORS['accent'], parent=None):
        super().__init__(parent)
        self.setFixedHeight(110)
        self.color = color

        layout = QVBoxLayout(self)
        layout.setContentsMargins(20, 16, 20, 16)
        layout.setSpacing(4)

        # 图标行
        icon_lbl = QLabel(icon)
        icon_lbl.setStyleSheet(f"font-size: 22px; background: transparent; border: none;")
        icon_lbl.setAlignment(Qt.AlignLeft)

        # 数值
        self.value_lbl = QLabel(value)
        self.value_lbl.setStyleSheet(
            f"font-size: 24px; font-weight: bold; color: {color};"
            f"background: transparent; border: none;")

        # 标签
        label_lbl = QLabel(label)
        label_lbl.setStyleSheet(
            f"font-size: 9pt; color: {COLORS['text_secondary']};"
            f"background: transparent; border: none;")

        layout.addWidget(icon_lbl)
        layout.addWidget(self.value_lbl)
        layout.addWidget(label_lbl)

        self.setStyleSheet(f"""
            StatCard {{
                background-color: {COLORS['bg_card']};
                border-radius: 12px;
                border: 1px solid {COLORS['border']};
            }}
            StatCard:hover {{
                border-color: {color};
            }}
        """)

    def set_value(self, v: str):
        self.value_lbl.setText(v)


class SidebarButton(QPushButton):
    """侧边栏导航按钮"""
    def __init__(self, icon: str, text: str, parent=None):
        super().__init__(parent)
        self.setText(f"  {icon}  {text}")
        self.setCheckable(True)
        self.setFixedHeight(44)
        self.setStyleSheet(f"""
            SidebarButton {{
                background-color: transparent;
                color: {COLORS['text_secondary']};
                border: none;
                border-radius: 8px;
                text-align: left;
                padding-left: 12px;
                font-size: 10pt;
            }}
            SidebarButton:hover {{
                background-color: {COLORS['bg_hover']};
                color: {COLORS['text_primary']};
            }}
            SidebarButton:checked {{
                background-color: {COLORS['bg_selected']};
                color: {COLORS['accent']};
                font-weight: bold;
            }}
        """)


class TitleBar(QWidget):
    """自定义无边框标题栏"""
    close_clicked = pyqtSignal()
    minimize_clicked = pyqtSignal()
    maximize_clicked = pyqtSignal()

    def __init__(self, title: str, parent=None):
        super().__init__(parent)
        self.setFixedHeight(48)
        self._drag_pos = None

        layout = QHBoxLayout(self)
        layout.setContentsMargins(16, 0, 8, 0)
        layout.setSpacing(8)

        # Logo + 标题
        logo = QLabel("🗂")
        logo.setStyleSheet("font-size: 18px; background: transparent; border: none;")
        title_lbl = QLabel(title)
        title_lbl.setStyleSheet(
            f"font-size: 12pt; font-weight: bold; color: {COLORS['text_primary']};"
            f"background: transparent; border: none;")

        layout.addWidget(logo)
        layout.addWidget(title_lbl)
        layout.addStretch()

        # 窗口控制按钮
        for symbol, signal, color in [
            ("─", self.minimize_clicked, COLORS['text_secondary']),
            ("□", self.maximize_clicked, COLORS['text_secondary']),
            ("✕", self.close_clicked, COLORS['accent_red']),
        ]:
            btn = QPushButton(symbol)
            btn.setFixedSize(36, 32)
            btn.setStyleSheet(f"""
                QPushButton {{
                    background: transparent;
                    color: {color};
                    border: none;
                    font-size: 13px;
                    border-radius: 6px;
                }}
                QPushButton:hover {{
                    background-color: {COLORS['bg_hover']};
                }}
            """)
            btn.clicked.connect(signal.emit)
            layout.addWidget(btn)

        self.setStyleSheet(f"""
            TitleBar {{
                background-color: {COLORS['bg_dark']};
                border-bottom: 1px solid {COLORS['border']};
            }}
        """)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            self._drag_pos = event.globalPos() - self.window().frameGeometry().topLeft()

    def mouseMoveEvent(self, event):
        if event.buttons() == Qt.LeftButton and self._drag_pos:
            self.window().move(event.globalPos() - self._drag_pos)

    def mouseReleaseEvent(self, event):
        self._drag_pos = None


class CategoryBadge(QLabel):
    """分类标签徽章"""
    def __init__(self, text: str, color: str = COLORS['accent'], parent=None):
        super().__init__(text, parent)
        self.setStyleSheet(f"""
            QLabel {{
                background-color: {color}33;
                color: {color};
                border: 1px solid {color}66;
                border-radius: 10px;
                padding: 2px 10px;
                font-size: 9pt;
                font-weight: bold;
            }}
        """)
        self.setFixedHeight(20)


class Divider(QFrame):
    """水平分隔线"""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFrameShape(QFrame.HLine)
        self.setStyleSheet(f"color: {COLORS['border']}; background: {COLORS['border']};")
        self.setFixedHeight(1)


class ToastNotification(QWidget):
    """右下角 Toast 通知"""
    def __init__(self, message: str, color: str = COLORS['accent_green'], parent=None):
        super().__init__(parent, Qt.FramelessWindowHint | Qt.ToolTip)
        self.setAttribute(Qt.WA_TranslucentBackground)
        self.setFixedSize(320, 54)

        layout = QHBoxLayout(self)
        layout.setContentsMargins(16, 0, 16, 0)

        dot = QLabel("●")
        dot.setStyleSheet(f"color: {color}; font-size: 12px; background: transparent; border: none;")
        msg = QLabel(message)
        msg.setStyleSheet(f"color: {COLORS['text_primary']}; background: transparent; border: none;")
        msg.setWordWrap(True)

        layout.addWidget(dot)
        layout.addWidget(msg, 1)

        self.setStyleSheet(f"""
            ToastNotification {{
                background-color: {COLORS['bg_card']};
                border: 1px solid {color}55;
                border-radius: 10px;
            }}
        """)

        # 自动消失动画
        from PyQt5.QtCore import QTimer
        QTimer.singleShot(2800, self.close)
