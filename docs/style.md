---
description: 根据项目代码规范格式化指定文件
argument-hint: <file path>
allowed-tools: Read, Edit, Write, Glob, Grep, Bash
---

## 你的任务

你是一个代码格式化助手。根据 `$ARGUMENTS` 指定的文件路径，读取文件并严格按照以下代码规范重新格式化。

## 代码规范

### 命名规范

1. **函数**: 统一使用小写下划线命名（`snake_case`），如 `set_font`、`add_task`、`size_constraints`
   - 成员函数、全局函数、辅助函数均遵循此规则
   - 简单属性读取直接使用属性名，不加 `get_` 前缀，如 `height()`、`size_constraints()`；对应 setter 使用 `set_` 前缀
   - 构造函数、析构函数和运算符遵循 C++ 语法；第三方接口调用与重写保留其要求的名称
2. **类型**: 大驼峰 (`ThreadPool`, `PlayerBar`, `SongInfo`)
3. **变量**: 下划线命名 (`player_bar`, `cover_image`)
   - 类的 private 和 protected 成员变量统一添加 `_` 后缀，不区分单词数量，如 `name_`、`size_constraints_`、`render_text_`；静态成员变量同样遵循此规则
   - public 数据成员、参数、局部变量和普通全局变量不加 `_` 后缀，如 `item_clicked`、`player_bar`、`cover_image`
   - 常量按下面的常量规则命名，不因访问权限添加 `_` 后缀
4. **常量**: `constexpr` 优先于 `const`，命名用 `k` 前缀大驼峰 (`kBarHeight`, `kCoverSize`)

### 渐进统一命名

- 在功能开发或修复时，顺带统一本次已经修改文件的命名，并同步必要的调用处与重写声明。
- 不为统一命名批量修改无关文件；命名调整随对应功能一起提交，避免全仓库重命名。

### 类成员排列规范

1. 类内的成员函数和成员变量统一按 `public` → `protected` → `private` 的访问权限顺序组织；没有成员的访问区块可省略。
2. `class` 的各访问区块必须显式写出访问说明符；`struct` 的默认 `public` 可省略，`protected` 和 `private` 仍需显式声明。同一访问权限的成员集中声明，避免交替出现多个相同访问区块。

### 注释规范

1. **所有函数**必须添加完整的 `Doxygen` 注释（`/** ... */`），函数有形参注释需要多行注释且形参需要加`Doxygen` 注释, 不同函数之间间隔一行
2. 成员函数仅需在**声明**处添加注释，实现处不重复
3. 全局/独立函数的 Doxygen 注释放在**定义**前
4. 代码关键实现需要添加行内注释
5. **所有变量**（类成员、全局变量、全局常量）右侧必须添加中文注释并列对齐

### 函数规范

1. 成员函数实现时该添加 `const` 的必须添加（`const` 成员函数、`const` 参数）
2. 函数定义的值参数如果不修改也要加 `const`
3. 不访问类资源的成员函数应改为独立函数，定义放在命名空间外部。
4. 文件内的局部辅助函数直接定义，无需单独写前向声明。文件先放所有类声明，再放局部辅助函数定义，最后放类成员函数的类外实现。
5. 类成员函数的声明和实现严格分离（除非特别短，如仅一行的内联 getter）。

   ```cpp
   class A {
   public:
     /** 创建实例。 */
     A();

   private:
     int value_; // 初始值
   };

   /** 返回默认值。 */
   int default_value() {
     return 0;
   }

   A::A() : value_(default_value()) {
   }
   ```

### 代码风格

1. 参数传递：合理规划，小类型按值传递，大对象按 const 引用传递
2. **代码尽量一行写完**，不要出现以下情况：
   ```cpp
   // BAD - 参数拆行但不够美观
   void PlayerBar::draw_volume_bar(SkCanvas *canvas, const float x, const float cy,
     const float w) const {
   ```
   除非多行参数排列非常整齐好看（如多个长参数对齐排列）
3. 变量声明尽量一行写完
4. 本项目全面采用 C++ 命名模块。仅供模块内部使用的辅助函数、类型和变量放在模块声明之后，省略 `export`，使用模块链接即可，无需使用匿名命名空间（`namespace {}`）。

## 执行流程

1. 读取 `$ARGUMENTS` 指定的文件
2. 逐项检查是否符合上述规范
3. 列出需要修改的项目
4. 执行修改
5. 最终验证所有规范已满足
