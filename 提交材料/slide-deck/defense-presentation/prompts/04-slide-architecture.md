Create a presentation slide image following these guidelines:

## Image Specifications

- **Type**: Presentation slide (Content — Architecture Diagram)
- **Aspect Ratio**: 16:9 (landscape)
- **Style**: Professional slide deck

## Core Principles

- Illustrative / stylized quality throughout - NO realistic or photographic elements
- NO slide numbers, page numbers, footers, headers, or logos
- Clean, precise architectural diagram aesthetic

## STYLE_INSTRUCTIONS

Design Aesthetic: Cinematic dark mode with industrial atmosphere. Engineering schematic precision on dark backgrounds.

Background: Deep Black #0a0a0a

Typography: Bold editorial Chinese characters in off-white #e8e8e8. Body labels in light gray #a0a0a0.

Color Palette:
- Primary Text: Off-White #e8e8e8
- Secondary Text: Light Gray #a0a0a0
- Background: Deep Black #0a0a0a
- Panel: Gunmetal #1a1a1a
- Accent Red: #aa3333 (connector lines and borders)
- Accent Amber: #ff9900 (key callout boxes)
- Accent Blue: #336699

Visual Elements: Wireframe architecture diagram. Sharp-cornered rectangular panels with red left-edge borders. Thin red connector lines with arrowheads. Amber callout boxes for design decisions. Engineering schematic precision.

Style Rules: Clean lines, aligned text, no decoration beyond function. Sharp industrial edges.

---

## SLIDE CONTENT

**Slide 4 of 8 — Architecture**
**Filename**: 04-slide-architecture.png

Narrative Goal: Demonstrate software engineering quality — MVC architecture, module decomposition, signal-slot data flow.

Key Content:
- Headline: 技术架构：MVC分层设计 (bold white)
- Sub-headline: 信号槽驱动的松耦合通信 · 单向数据流 (gray)
- Architecture layers (top to bottom):
  - UI层 (View): MenuWidget / CharacterCreateWidget / GameWidget / SaveLoadDialog
  - 路由层 (Router): MainWindow — QStackedWidget页面管理 + 信号转发中枢
  - 逻辑层 (Controller): GameEngine — 中央调度器 · 随机判定 · 选择处理 · 状态管理
  - 数据层 (Model): Player / StoryNode / ScenarioBase — 节点图 · 玩家状态 · JSON序列化
  - 基础设施: SaveManager (JSON持久化) · MusicPlayer (QMediaPlayer音频)

Visual: Clean wireframe diagram on dark background. Four horizontal layer panels rendered as gunmetal-gray rectangles with thin red left-edge borders, stacked vertically. Each layer has its white label and component names in light gray. Thin red connector lines with arrowheads show Qt signal flow between layers. Two amber (#ff9900) callout boxes highlight: "信号槽解耦" and "场景数据不随存档序列化". Engineering schematic precision — aligned, functional, no decoration.

Layout (hierarchical-layers): Four stacked horizontal layers from top (UI) to bottom (Infrastructure). Wireframe aesthetic with minimal connector lines. Amber callout annotations.
