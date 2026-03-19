# -*- coding: utf-8 -*-
"""
CCdesk 核心整理引擎
负责：文件扫描、分类、移动、撤销、历史记录
"""

import os
import shutil
import json
import time
import hashlib
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Optional, Tuple


# ─────────────────────────────────────────────
# 默认分类规则
# ─────────────────────────────────────────────
DEFAULT_RULES: Dict[str, Dict] = {
    "图片": {
        "folder": "图片",
        "extensions": [".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp",
                       ".tiff", ".tif", ".ico", ".svg", ".raw", ".heic"],
        "color": "#4CAF50",
        "icon": "🖼️",
    },
    "文档": {
        "folder": "文档",
        "extensions": [".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
                       ".pdf", ".txt", ".md", ".csv", ".rtf", ".odt", ".wps"],
        "color": "#2196F3",
        "icon": "📄",
    },
    "视频": {
        "folder": "视频",
        "extensions": [".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv",
                       ".rmvb", ".m4v", ".3gp", ".ts", ".webm"],
        "color": "#F44336",
        "icon": "🎬",
    },
    "音乐": {
        "folder": "音乐",
        "extensions": [".mp3", ".wav", ".flac", ".aac", ".ogg", ".wma",
                       ".m4a", ".ape", ".opus"],
        "color": "#9C27B0",
        "icon": "🎵",
    },
    "压缩包": {
        "folder": "压缩包",
        "extensions": [".zip", ".rar", ".7z", ".tar", ".gz", ".bz2",
                       ".xz", ".iso", ".cab"],
        "color": "#FF9800",
        "icon": "📦",
    },
    "程序": {
        "folder": "程序",
        "extensions": [".exe", ".msi", ".bat", ".cmd", ".ps1", ".sh",
                       ".apk", ".dmg", ".pkg"],
        "color": "#607D8B",
        "icon": "⚙️",
    },
    "代码": {
        "folder": "代码",
        "extensions": [".py", ".js", ".ts", ".html", ".css", ".java",
                       ".cpp", ".c", ".h", ".cs", ".php", ".go", ".rs",
                       ".vue", ".jsx", ".tsx", ".json", ".xml", ".yaml",
                       ".yml", ".toml", ".ini", ".cfg", ".sql"],
        "color": "#00BCD4",
        "icon": "💻",
    },
    "其他": {
        "folder": "其他",
        "extensions": [],   # 兜底分类
        "color": "#9E9E9E",
        "icon": "📁",
    },
}


class FileInfo:
    """扫描到的文件信息"""
    def __init__(self, path: str):
        self.path = Path(path)
        self.name = self.path.name
        self.ext = self.path.suffix.lower()
        self.size = self.path.stat().st_size if self.path.exists() else 0
        self.mtime = datetime.fromtimestamp(
            self.path.stat().st_mtime) if self.path.exists() else datetime.now()
        self.category = ""
        self.target_path: Optional[Path] = None

    @property
    def size_str(self) -> str:
        for unit in ["B", "KB", "MB", "GB"]:
            if self.size < 1024:
                return f"{self.size:.1f} {unit}"
            self.size /= 1024
        return f"{self.size:.1f} TB"

    def to_dict(self) -> dict:
        return {
            "path": str(self.path),
            "name": self.name,
            "ext": self.ext,
            "size": self.size,
            "mtime": self.mtime.isoformat(),
            "category": self.category,
            "target_path": str(self.target_path) if self.target_path else None,
        }


class OrganizeRecord:
    """一次整理操作的记录，支持撤销"""
    def __init__(self, task_id: str, source_dir: str, mode: str):
        self.task_id = task_id
        self.source_dir = source_dir
        self.mode = mode                    # "type" | "date" | "custom"
        self.timestamp = datetime.now()
        self.moves: List[Tuple[str, str]] = []  # (src, dst)
        self.created_dirs: List[str] = []
        self.success_count = 0
        self.skip_count = 0
        self.error_count = 0

    def add_move(self, src: str, dst: str):
        self.moves.append((src, dst))

    def to_dict(self) -> dict:
        return {
            "task_id": self.task_id,
            "source_dir": self.source_dir,
            "mode": self.mode,
            "timestamp": self.timestamp.isoformat(),
            "moves": self.moves,
            "created_dirs": self.created_dirs,
            "success_count": self.success_count,
            "skip_count": self.skip_count,
            "error_count": self.error_count,
        }

    @classmethod
    def from_dict(cls, d: dict) -> "OrganizeRecord":
        r = cls(d["task_id"], d["source_dir"], d["mode"])
        r.timestamp = datetime.fromisoformat(d["timestamp"])
        r.moves = [tuple(m) for m in d["moves"]]
        r.created_dirs = d.get("created_dirs", [])
        r.success_count = d.get("success_count", 0)
        r.skip_count = d.get("skip_count", 0)
        r.error_count = d.get("error_count", 0)
        return r


class OrganizeEngine:
    """核心整理引擎"""

    def __init__(self, config_dir: str = None):
        if config_dir is None:
            config_dir = str(Path.home() / ".ccdesk")
        self.config_dir = Path(config_dir)
        self.config_dir.mkdir(parents=True, exist_ok=True)

        self.rules_file = self.config_dir / "rules.json"
        self.history_file = self.config_dir / "history.json"
        self.settings_file = self.config_dir / "settings.json"

        self.rules = self._load_rules()
        self.history: List[OrganizeRecord] = self._load_history()
        self.settings = self._load_settings()

    # ── 规则管理 ──────────────────────────────
    def _load_rules(self) -> Dict:
        if self.rules_file.exists():
            try:
                with open(self.rules_file, "r", encoding="utf-8") as f:
                    return json.load(f)
            except Exception:
                pass
        return DEFAULT_RULES.copy()

    def save_rules(self):
        with open(self.rules_file, "w", encoding="utf-8") as f:
            json.dump(self.rules, f, ensure_ascii=False, indent=2)

    def reset_rules(self):
        self.rules = DEFAULT_RULES.copy()
        self.save_rules()

    # ── 设置管理 ──────────────────────────────
    def _load_settings(self) -> dict:
        default = {
            "auto_organize": False,
            "auto_interval_hours": 24,
            "auto_target": str(Path.home() / "Desktop"),
            "auto_mode": "type",
            "skip_shortcuts": True,
            "skip_system_files": True,
            "handle_duplicate": "rename",  # rename | skip | overwrite
        }
        if self.settings_file.exists():
            try:
                with open(self.settings_file, "r", encoding="utf-8") as f:
                    saved = json.load(f)
                    default.update(saved)
            except Exception:
                pass
        return default

    def save_settings(self):
        with open(self.settings_file, "w", encoding="utf-8") as f:
            json.dump(self.settings, f, ensure_ascii=False, indent=2)

    # ── 历史记录 ──────────────────────────────
    def _load_history(self) -> List[OrganizeRecord]:
        if self.history_file.exists():
            try:
                with open(self.history_file, "r", encoding="utf-8") as f:
                    data = json.load(f)
                    return [OrganizeRecord.from_dict(d) for d in data]
            except Exception:
                pass
        return []

    def _save_history(self):
        # 只保留最近 50 条
        records = self.history[-50:]
        with open(self.history_file, "w", encoding="utf-8") as f:
            json.dump([r.to_dict() for r in records], f,
                      ensure_ascii=False, indent=2)

    # ── 文件扫描 ──────────────────────────────
    def scan(self, directory: str, recursive: bool = False) -> List[FileInfo]:
        """扫描目录，返回文件列表（不含子目录本身）"""
        directory = Path(directory)
        files = []
        try:
            if recursive:
                items = directory.rglob("*")
            else:
                items = directory.iterdir()

            for item in items:
                if not item.is_file():
                    continue
                if self.settings.get("skip_shortcuts") and item.suffix.lower() == ".lnk":
                    continue
                if self.settings.get("skip_system_files") and item.name.startswith("."):
                    continue
                fi = FileInfo(str(item))
                fi.category = self._classify_by_type(fi.ext)
                files.append(fi)
        except PermissionError:
            pass
        return files

    def _classify_by_type(self, ext: str) -> str:
        for cat_name, cat_info in self.rules.items():
            if cat_name == "其他":
                continue
            if ext.lower() in cat_info.get("extensions", []):
                return cat_name
        return "其他"

    def _classify_by_date(self, fi: FileInfo) -> str:
        return fi.mtime.strftime("%Y年%m月")

    # ── 预览整理计划 ──────────────────────────
    def preview(self, directory: str, mode: str = "type") -> List[FileInfo]:
        """
        生成整理预览，返回带有 target_path 的 FileInfo 列表
        mode: "type" | "date" | "custom"
        """
        files = self.scan(directory)
        base = Path(directory)

        for fi in files:
            if mode == "type":
                cat = fi.category
                folder_name = self.rules.get(cat, {}).get("folder", cat)
                fi.target_path = base / folder_name / fi.name
            elif mode == "date":
                date_folder = self._classify_by_date(fi)
                fi.target_path = base / date_folder / fi.name
            else:
                fi.target_path = base / self.rules.get(
                    fi.category, {}).get("folder", fi.category) / fi.name

        return files

    # ── 执行整理 ──────────────────────────────
    def organize(
        self,
        directory: str,
        mode: str = "type",
        progress_callback=None,
    ) -> OrganizeRecord:
        """
        执行整理，返回操作记录
        progress_callback(current, total, filename) → 用于更新进度条
        """
        task_id = hashlib.md5(
            f"{directory}{time.time()}".encode()).hexdigest()[:8]
        record = OrganizeRecord(task_id, directory, mode)
        files = self.preview(directory, mode)
        total = len(files)

        for i, fi in enumerate(files):
            if progress_callback:
                progress_callback(i + 1, total, fi.name)

            if fi.target_path is None or fi.path == fi.target_path:
                record.skip_count += 1
                continue

            try:
                dst = self._resolve_duplicate(fi.path, fi.target_path)
                dst.parent.mkdir(parents=True, exist_ok=True)
                if str(dst.parent) not in record.created_dirs:
                    record.created_dirs.append(str(dst.parent))

                shutil.move(str(fi.path), str(dst))
                record.add_move(str(fi.path), str(dst))
                record.success_count += 1
            except Exception as e:
                print(f"[ERROR] 移动失败 {fi.path}: {e}")
                record.error_count += 1

        self.history.append(record)
        self._save_history()
        return record

    def _resolve_duplicate(self, src: Path, dst: Path) -> Path:
        """处理目标路径重名"""
        mode = self.settings.get("handle_duplicate", "rename")
        if not dst.exists():
            return dst
        if mode == "skip":
            return src  # 跳过（保持原位）
        if mode == "overwrite":
            return dst
        # rename: 追加序号
        stem = dst.stem
        suffix = dst.suffix
        parent = dst.parent
        counter = 1
        while True:
            new_dst = parent / f"{stem}_{counter}{suffix}"
            if not new_dst.exists():
                return new_dst
            counter += 1

    # ── 撤销整理 ──────────────────────────────
    def undo(self, record: OrganizeRecord, progress_callback=None) -> int:
        """撤销一次整理操作，返回成功恢复的文件数"""
        success = 0
        total = len(record.moves)
        for i, (src, dst) in enumerate(reversed(record.moves)):
            if progress_callback:
                progress_callback(i + 1, total, Path(dst).name)
            try:
                dst_path = Path(dst)
                src_path = Path(src)
                if dst_path.exists():
                    src_path.parent.mkdir(parents=True, exist_ok=True)
                    shutil.move(str(dst_path), str(src_path))
                    success += 1
            except Exception as e:
                print(f"[ERROR] 撤销失败 {dst}: {e}")

        # 清理空目录
        for d in record.created_dirs:
            try:
                dp = Path(d)
                if dp.exists() and not any(dp.iterdir()):
                    dp.rmdir()
            except Exception:
                pass

        # 从历史中移除
        if record in self.history:
            self.history.remove(record)
        self._save_history()
        return success

    # ── 统计信息 ──────────────────────────────
    def get_stats(self, directory: str) -> dict:
        files = self.scan(directory)
        stats = {"total": len(files), "categories": {}}
        for fi in files:
            cat = fi.category
            if cat not in stats["categories"]:
                stats["categories"][cat] = {"count": 0, "size": 0}
            stats["categories"][cat]["count"] += 1
            stats["categories"][cat]["size"] += fi.path.stat().st_size if fi.path.exists() else 0
        return stats
