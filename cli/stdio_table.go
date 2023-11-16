// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"strings"

	"github.com/pterm/pterm"
)

type Table struct {
	title  string
	header []string
	rows   [][]string
}

func (t *Table) SetTitle(title string) *Table {
	t.title = title
	return t
}

func (t *Table) SetHeader(fields ...string) *Table {
	t.header = fields
	return t
}

func (t *Table) AddRow(fields ...string) *Table {
	t.rows = append(t.rows, fields)
	return t
}

func (t *Table) Println() {
	pTable := pterm.DefaultTable
	// pTable.HeaderStyle = secondaryStyle

	data := t.rows

	if t.header != nil {
		pTable = *pTable.WithHasHeader()
		pTable = *pTable.WithHeaderRowSeparator("⎯")
		data = append([][]string{t.header}, data...)
	}

	for i, row := range data {
		for j, cell := range row {
			data[i][j] = trimNewlines(cell)
		}
	}

	tableStr, err := pTable.WithData(data).Srender()
	if err != nil {
		dieE(err)
	}

	tableLines := strings.Split(tableStr, "\n")
	maxLineLen := 0

	for _, line := range tableLines {
		lineLen := runeLength(line)

		if lineLen > maxLineLen {
			maxLineLen = lineLen
		}
	}

	title := ""
	if t.title != "" {
		title = " " + t.title + " "
	}

	borderStyle := pterm.ThemeDefault.TableSeparatorStyle
	pterm.Println(borderStyle.Sprint("⎯⎯⎯") + pterm.ThemeDefault.PrimaryStyle.Sprint(title) + borderStyle.Sprint(strings.Repeat("⎯", maxLineLen-runeLength(title)-3)))
	pterm.Println(tableStr)
	pterm.Println(borderStyle.Sprint(strings.Repeat("⎯", maxLineLen)))
}
