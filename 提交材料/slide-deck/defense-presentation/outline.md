# Slide Deck Outline

**Topic**: 基于C++与Qt6的交互式文字冒险游戏 —— 《钢铁意志：第三帝国的黄昏》答辩演示
**Style**: dark-atmospheric
**Dimensions**: clean + dark + editorial + balanced
**Audience**: experts (答辩委员会)
**Language**: zh
**Slide Count**: 8 slides
**Generated**: 2026-06-14

---

<STYLE_INSTRUCTIONS>
Design Aesthetic: Cinematic dark mode with military-industrial atmosphere. Deep blacks and gunmetal grays with blood-red and amber accents creating dramatic contrast. Clean geometric precision with editorial typography — evoking wartime telegrams, military briefings, and industrial machinery. The visual language speaks of iron, fire, and shadow.

Background:
  Texture: Clean, subtle radial gradient from lighter gunmetal center (#1a1a1a) to deep black edges (#0a0a0a), with optional faint vertical line pattern suggesting riveted steel plates
  Base Color: Deep Black (#0a0a0a)

Typography:
  Headlines: Bold editorial sans-serif in white/off-white, high contrast against dark background, letter-spacing for authority. Chinese headlines use geometric sans-serif with sharp terminals. For image generation: describe as "bold white Chinese characters with slight letter-spacing, authoritative and crisp"
  Body: Clean readable sans-serif in light gray (#c0c0c0), medium weight. For image generation: describe as "clean gray Chinese text, well-spaced, readable against dark background"

Color Palette:
  Primary Text: Off-White (#e8e8e8) - headlines and key labels
  Secondary Text: Light Gray (#a0a0a0) - body text and descriptions
  Background: Deep Black (#0a0a0a) - primary background
  Panel: Gunmetal (#1a1a1a) - card and panel backgrounds
  Accent 1 (Blood Red): #aa3333 - emphasis, borders, combat elements
  Accent 2 (Iron Red): #cc4444 - hover states, danger markers
  Accent 3 (Amber Orange): #ff9900 - key data highlights, scenario markers
  Accent 4 (Cold Blue): #336699 - morale/secondary systems
  Subtle: Dark Gray (#2d2d2d) - dividers, borders, subtle lines

Visual Elements:
  - Military-industrial geometric frames: sharp-cornered rectangles with thin iron-red borders
  - Subtle horizontal rule lines evoking military documents and telegrams
  - Code blocks rendered as glowing amber text on near-black panels (terminal aesthetic)
  - Architecture diagrams as clean white-on-dark wireframe boxes with red connector lines
  - Progress bars in blood-red and cold-blue on gunmetal tracks
  - Faint geometric grid or rivet-line pattern in background for depth
  - Key numbers/statistics rendered large in amber orange as focal points

Density Guidelines:
  - Content per slide: 2-3 key points with supporting detail (balanced)
  - Whitespace: generous margins (10-12% from edges), breathing room between sections
  - Maximum 2 code blocks per slide, maximum 4 bullet points

Style Rules:
  Do:
    - Maintain high contrast for readability (white/gray on black)
    - Use red accents sparingly for emphasis on combat/risk elements
    - Create depth with 3-4 layers of dark gray panels
    - Keep text crisp and aligned with military precision
    - Use amber/orange for data highlights and key numbers
  Don't:
    - Overuse neon or glow effects (keep industrial, not cyberpunk)
    - Use bright backgrounds or white panels
    - Add cluttered decorative elements
    - Use rounded corners (keep sharp, industrial edges)
    - Add slide numbers, footers, or logos
</STYLE_INSTRUCTIONS>

---

## Slide 1 of 8

**Type**: Cover
**Filename**: 01-slide-cover.png

// NARRATIVE GOAL
Establish the project identity — a serious, technically-substantial game development internship project with a historical WWII theme. Set the cinematic, military-industrial tone.

// KEY CONTENT
Headline: 钢铁意志：第三帝国的黄昏
Sub-headline: 基于C++17与Qt6的交互式文字冒险游戏 — 实习答辩

// VISUAL
Dramatic cover composition. Deep black background with subtle radial glow in center-upper area suggesting distant firelight or explosions. The main title rendered large in bold white Chinese characters, centered, with slight authoritative letter-spacing. Below it, a thin blood-red horizontal line spans about 60% width. The subtitle in smaller gray text beneath. At the very bottom, faint silhouette-like outlines of WWII-era elements — tank treads, aircraft wings, submarine conning tower — barely visible as dark-on-dark shapes along the lower edge, creating atmospheric depth without distraction. A single amber accent element (perhaps a subtle dot or bracket) beside the title adds a focal point.

// LAYOUT
Layout: title-hero
Centered composition. Title dominates upper-middle area (approx 40% from top). Decorative red rule line separates title from subtitle. Minimal bottom elements provide atmospheric depth. Wide margins, clean and authoritative.

---

## Slide 2 of 8

**Type**: Content (Agenda)
**Filename**: 02-slide-agenda.png

// NARRATIVE GOAL
Give the defense committee a clear roadmap of the presentation structure — what will be covered and in what order.

// KEY CONTENT
Headline: 答辩纲要
Body: Four numbered sections arranged vertically or in a grid:
1. 项目概述 — 背景、目标与技术栈
2. 技术架构 — MVC分层设计与模块划分
3. 核心系统实现 — 场景图、骰子战斗、存档、打字机
4. 总结与展望 — 项目收获、局限分析与改进方向

// VISUAL
Clean dark gunmetal background. The headline "答辩纲要" in bold white at top-left. Below it, four numbered items arranged vertically on the left side, each with a small blood-red accent number and gray descriptive text. On the right side, a subtle vertical timeline or progress bar in dark red suggests the presentation flow from top to bottom. Minimal, structured, military-briefing aesthetic. Thin horizontal rules separate each item slightly.

// LAYOUT
Layout: agenda
Two-column split: left side (60%) contains the numbered agenda items, right side (40%) shows a subtle vertical timeline indicator. Clean geometric alignment.

---

## Slide 3 of 8

**Type**: Content
**Filename**: 03-slide-project-overview.png

// NARRATIVE GOAL
Present the overall internship task — what the project is, its scope, and the technology foundation. Establish credibility by showing the scale of work completed.

// KEY CONTENT
Headline: 项目总体任务
Sub-headline: 面向对象程序设计实践课程 — 游戏开发项目
Body:
- 项目类型：二战题材交互式文字冒险游戏，聚焦战争残酷与道德困境
- 核心数据：~3000行C++17代码 · 28个源文件 · 5大战役 · 129个故事节点 · 15+种结局
- 技术栈：C++17 + Qt6 (Core/Widgets/Multimedia) + CMake 3.16+ + JSON持久化
- 六大职业系统：步兵/坦克兵/战斗机飞行员/轰炸机飞行员/潜艇驾驶员/战列舰水兵

// VISUAL
Layout structured like a military intelligence briefing card. Headline in bold white at top. Below, a gunmetal panel (#1a1a1a) with thin iron-red border contains the project statistics rendered in a clean grid — key numbers (3000行, 28文件, 5战役, 129节点, 15结局) highlighted in amber orange. Below that, the technology stack listed in gray with subtle icon-like markers. On the right edge, a faint vertical list of the six classes in small text, red-accented. The overall feel is a "mission briefing document" — structured, data-forward, authoritative.

// LAYOUT
Layout: dashboard
Key statistics as prominent numbers in amber. Supporting text in gray. Grid-aligned, military-briefing precision.

---

## Slide 4 of 8

**Type**: Content
**Filename**: 04-slide-architecture.png

// NARRATIVE GOAL
Demonstrate the software engineering quality of the project — show the MVC architecture, module decomposition, and data flow design that makes the codebase maintainable and extensible.

// KEY CONTENT
Headline: 技术架构：MVC分层设计
Sub-headline: 信号槽驱动的松耦合通信 · 单向数据流
Body (organized in visual architecture diagram):
- UI层 (View)：MenuWidget / CharacterCreateWidget / GameWidget / SaveLoadDialog
- 路由层 (Router)：MainWindow — QStackedWidget页面管理 + 信号转发中枢
- 逻辑层 (Controller)：GameEngine — 中央调度器 · 骰子判定 · 选择处理 · 状态管理
- 数据层 (Model)：Player / StoryNode / ScenarioBase — 节点图 · 玩家状态 · JSON序列化
- 基础设施：SaveManager (JSON持久化) · MusicPlayer (QMediaPlayer音频)

// VISUAL
A clean wireframe architecture diagram on dark background. Four horizontal layers rendered as gunmetal-gray rectangular panels (#1a1a1a) with thin iron-red left-edge borders, stacked vertically. Each layer has its label in white and component names in light gray. Thin red connector lines with arrowheads show the Qt signal flow between layers. The entire diagram has the precision of an engineering schematic — clean lines, aligned text, no decoration beyond the functional. Amber callout boxes highlight key design decisions ("信号槽解耦", "场景数据不随存档序列化").

// LAYOUT
Layout: hierarchical-layers
Four stacked horizontal layers from top (UI) to bottom (Infrastructure). Clean wireframe aesthetic with minimal connector lines.

---

## Slide 5 of 8

**Type**: Content
**Filename**: 05-slide-core-systems.png

// NARRATIVE GOAL
Showcase the four most technically substantive core systems. Each deserves its own visual block — demonstrate depth of implementation.

// KEY CONTENT
Headline: 核心系统实现
Body (four system blocks in a 2×2 grid):

Block 1 — 场景图与叙事系统：有向图结构 · StoryNode/Choice双数据结构 · Narrative/Choice/Ending三种节点类型 · 职业限制+标志条件双重过滤 · 每战役10-28个分支节点

Block 2 — D100骰子战斗系统：QRandomGenerator::bounded(1,101) · 职业匹配+20加成 · 阈值梯度设计(40-90) · 成功/失败双路径跳转 · HP+士气双重死亡判定

Block 3 — JSON存档系统：QJsonDocument序列化 · 1自动槽+3手动槽 · %APPDATA%标准路径 · 自动存档(每次节点切换) · 默认值容错恢复

Block 4 — 打字机效果：QTimer每20ms逐字渲染 · 事件过滤器拦截点击跳过 · 打字期间禁用按钮 · 强制滚动到底部

// VISUAL
2×2 grid of gunmetal panels, each with a thin iron-red top border. Each panel has a small bold white system name as header, and 3-4 gray bullet points of key technical details. The top-left panel (scene system) has a subtle miniature node-graph diagram in its corner. The top-right panel (dice combat) has a small amber "D100" indicator. The bottom-left panel (save system) shows miniature JSON structure in amber-on-black terminal style. The bottom-right panel (typewriter) has subtle text-progression dots. Clean separation, equal visual weight across all four.

// LAYOUT
Layout: bento-grid
Four equal-sized panels in 2×2 grid. Each self-contained with header + technical bullets + miniature visual indicator.

---

## Slide 6 of 8

**Type**: Content
**Filename**: 06-slide-ui-and-effects.png

// NARRATIVE GOAL
Present the UI/UX design and atmospheric effects — the "polish" layer that elevates the project from functional to immersive.

// KEY CONTENT
Headline: 界面设计与沉浸体验
Sub-headline: 军工战损风主题 · 四界面QStackedWidget管理
Body (two columns):

左列 — 视觉设计系统：
- 暗色四层纵深：#121212 → #181818 → #1c1c1c → #242424
- 功能色语义化：血红HP(#992222) · 冷蓝士气(#226699) · 红调战斗按钮 · 全无圆角(border-radius:0)
- 纯QSS实现 · 零外部图片依赖 · 等宽回退字体(monospace电报感)

右列 — 游戏内效果：
- 打字机逐字渲染：20ms/字符 · 点击跳过 · 完成前锁定操作
- 选项智能过滤：职业不匹配/标志不满足自动隐去按钮
- 音乐系统：键-路径映射 · 播放去重 · 文件缺失静默容错 · 循环播放
- 战斗结果弹窗：QMessageBox区分成功/失败视觉反馈

// VISUAL
Two-column layout. Left column shows the visual design system — a small mockup of the color palette as four stacked color swatches (black → gunmetal → red → amber), with labels. Right column shows UI interaction flow — simple wireframe illustrations of the typewriter animation progression and the option filtering logic. Both columns rendered on gunmetal panel backgrounds with iron-red accent lines. The overall feel is "design documentation" — precise, annotated, systematic.

// LAYOUT
Layout: two-columns
Balanced left-right split. Left: visual design specs + palette. Right: interaction effects + system flow. Equal visual weight.

---

## Slide 7 of 8

**Type**: Content
**Filename**: 07-slide-problems-and-improvements.png

// NARRATIVE GOAL
Honestly acknowledge current limitations while demonstrating forward-thinking — show the committee you understand software engineering trade-offs and have a vision for improvement.

// KEY CONTENT
Headline: 当前局限与改进方向
Body (two-column binary comparison):

左列 — 现有问题（红调标注）：
- 无国际化(i18n)：文本硬编码中文，未使用Qt Linguist
- 音乐文件未随源码分发：因版权原因，需用户自行准备MP3
- 固定窗口尺寸(850×650)：透明按钮覆盖层硬编码，窗口缩放时偏移
- 战斗选项缺少不可选原因提示：标志/职业不满足时静默隐藏
- 成就/统计系统缺失：道德标志无统一汇总面板

右列 — 改进方向（蓝调标注）：
- Qt Linguist .ts文件实现多语言
- 程序化音频或静默模式作为fallback
- 响应式布局重构 + 全屏模式
- UI提示"需要XX条件"的tooltip反馈
- 成就面板 + 全结局解锁进度追踪
- 扩展场景分支深度，让叙事标志更显著影响剧情

// VISUAL
Binary comparison layout. Left column (problems) has iron-red tinted panels and red accent markers. Right column (improvements) has cold-blue tinted panels and blue accent markers. A subtle central dividing line separates the two. Each item is a compact labeled block. At the top, "现有问题" in red and "改进方向" in blue create clear visual separation. The tone is analytical, not apologetic — a technical assessment by an engineer who understands their codebase.

// LAYOUT
Layout: binary-comparison
Two columns: Problems (left, red-tinted) vs Improvements (right, blue-tinted). Clear visual contrast between current state and future direction.

---

## Slide 8 of 8

**Type**: Back Cover
**Filename**: 08-slide-back-cover.png

// NARRATIVE GOAL
Close with a reflective summary of what was learned and achieved. Leave the committee with a sense of the project's completeness and the presenter's growth as a software engineer.

// KEY CONTENT
Headline: 总结与收获
Body (three key takeaways):
1. 面向对象设计实践 — 从抽象基类到具体场景继承，深入理解继承、多态与接口隔离原则
2. 软件架构能力 — MVC分层、信号槽解耦、数据驱动设计，维护3000行代码项目的清晰结构
3. 系统工程全流程 — CMake构建配置 → Qt6集成 → JSON持久化 → 发布打包

Closing quote: "愿世界永无战争。"

// VISUAL
A reflective, quieter variation of the cover composition. Deep black background with a very subtle warm amber glow suggesting dawn or embers — not fire, but the quiet after. The headline "总结与收获" in bold white, centered-upper. Below it, three numbered takeaways arranged vertically, each with a small amber marker and gray descriptive text, clean and readable. The closing quote rendered in italic gray text at the bottom, smaller, like a whispered epilogue. No aggressive red elements — this is the reflection slide. The visual language shifts from "briefing" to "contemplation."

// LAYOUT
Layout: quote-callout
Headline + three focused points + resonant closing quote. Clean vertical flow, generous whitespace. Atmospheric warmth in the background glow — amber instead of red.
