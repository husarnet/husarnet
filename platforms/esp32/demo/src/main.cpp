// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <Arduino.h>

#define LED_BUILTIN 2

void run_husarnet();

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  run_husarnet();
}

void loop()
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}
