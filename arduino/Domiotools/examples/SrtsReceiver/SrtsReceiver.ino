/* Copyright (C) 2015 Sylvain Afchain
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "srts.h"

unsigned long last_time;
unsigned int last_type;

void setup()
{
  pinMode(2, INPUT);
  Serial.begin(9600);

  last_time = micros();
  last_type = HIGH;
}

void loop()
{
  struct srts_payload payload;
  unsigned int type, duration;
  unsigned long now;
  int rc = 0;

  type = digitalRead(2);
  if (type != last_type) {
      now = micros();
      duration = now - last_time;
      last_time = now;

      rc = srts_receive(2, last_type, duration, &payload);
      if (rc == 1) {
        Serial.println("Srts message received:");
        Serial.println("  key: " + String(payload.key));
        Serial.println("  address: " + String(srts_get_address(&payload)));
        Serial.println("  ctrl: " + String(payload.ctrl));
        Serial.println("  code: " + String(payload.code));
      }

      last_type = type;
  }
}
