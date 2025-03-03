# 提取 word 模板

---

## 1. 准备一个参考 Word 文档

- 创建一个 Word 文档（例如 `reference.docx`），设置好你想要的样式（如标题、正文、引用等）。
- 确保文档中包含你希望保留的格式（如页眉、页脚、字体、段落样式等）。

---

## 2. 使用 Pandoc 提取模板

Pandoc 可以通过 `--reference-doc` 选项将一个 Word 文档作为模板提取出来。运行以下命令：

```bash
pandoc -o template.docx --print-default-data-file reference.docx
```

- `reference.docx`：你准备的参考 Word 文档。
- `template.docx`：生成的模板文件。

---

## 3. 使用生成的模板

生成的 `template.docx` 可以作为模板用于后续的文档生成。例如，将一个 Markdown 文件转换为 Word 文档时，使用以下命令：

```bash
pandoc input.md -o output.docx --reference-doc=template.docx
```

- `input.md`：你的 Markdown 文件。
- `output.docx`：生成的 Word 文档。
- `--reference-doc=template.docx`：指定生成的模板文件。

---

## 4. 检查模板内容

- 打开生成的 `template.docx`，检查样式是否符合预期。
- 如果样式未正确提取，可以手动调整模板中的样式名称，确保与 Pandoc 默认样式名称匹配。

---

## 5. 手动调整模板（可选）

如果生成的模板不完全符合需求，可以手动调整：

- 打开 `template.docx`，修改样式（如字体、段落格式、页眉页脚等）。
- 保存修改后的模板，重新使用它生成文档。

---

## 6. 注意事项

- Pandoc 提取模板时，会尽量保留参考文档中的样式和格式，但某些复杂格式（如表格样式、特殊字体）可能需要手动调整。
- 确保参考文档中的样式名称与 Pandoc 默认样式名称一致，以避免样式不匹配的问题。

---

通过以上步骤，你可以根据现有的 Word 文档生成一个模板，并使用它来生成符合格式要求的 Word 文档。
