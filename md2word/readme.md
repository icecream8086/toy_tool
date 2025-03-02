# Markdown 转 Word 转换器 (md2doc)

## 用法

`md2doc <输入路径> [pandoc_选项...]`

## 参数

- `<输入路径>`

  Markdown 文件或包含 Markdown 文件的目录的路径。

- `[pandoc_选项]`

  可选的 Pandoc 转换选项 (例如, `--toc`, `-s`)。 有关选项的完整列表，请参阅 [Pandoc 的文档](https://www.google.com/url?sa=E&source=gmail&q=https://pandoc.org/MANUAL.html)。

## 示例

- 将单个 Markdown 文件转换为 Word:

  `md2doc document.md --toc`

- 将目录中所有 Markdown 文件转换为 Word:

  `md2doc ./notes -s --reference-doc template.docx`

- 显示帮助信息:

  `md2doc -h`

  `md2doc --help`

## 注意事项

- 输出文件保存在与输入文件相同的目录中。
- 输出文件名与输入文件名相同，但扩展名为 `.docx`。
- 确保 [Pandoc](https://www.google.com/url?sa=E&source=gmail&q=https://pandoc.org/) 已安装并可在系统的 PATH 环境变量中访问。

## 帮助

要显示此帮助信息，请使用 `-h` 或 `--help` 标志:

`md2doc -h`

`md2doc --help`

如果运行 `md2doc` 时不带任何参数，将显示简要用法消息，提示您使用 `-h` 获取更详细的帮助。
