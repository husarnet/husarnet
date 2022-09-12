// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"strings"

	"github.com/mattn/go-runewidth"
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

	data := t.rows

	if t.header != nil {
		pTable = *pTable.WithHasHeader()
		data = append([][]string{t.header}, data...)
	}

	tableStr, err := pTable.WithData(data).Srender()
	if err != nil {
		dieE(err)
	}

	tableLines := strings.Split(tableStr, "\n")
	maxLineLen := 0

	for _, line := range tableLines {
		lineLen := runewidth.StringWidth(pterm.RemoveColorFromString(line))

		if lineLen > maxLineLen {
			maxLineLen = lineLen
		}
	}

	title := ""
	if t.title != "" {
		title = " " + t.title + " "
	}

	gray := pterm.NewStyle(pterm.FgGray)
	pterm.Println(gray.Sprint("⎯⎯⎯") + title + gray.Sprint(strings.Repeat("⎯", maxLineLen-len(title)-3)))
	pterm.Println(tableStr)
	pterm.Println(gray.Sprint(strings.Repeat("⎯", maxLineLen)))
}
