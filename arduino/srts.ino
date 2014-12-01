enum COMMAND {
    UNKNOWN = 0,
    MY = 1,
    UP,
    MY_UP,
    DOWN,
    MY_DOWN,
    UP_DOWN,
    PROG = 8,
    SUN_FLAG,
    FLAG,
};

struct srts_payload {
    unsigned char key;
    unsigned char checksum :4;
    unsigned char ctrl :4;
    unsigned short code;
    struct address {
        unsigned char byte1;
        unsigned char byte2;
        unsigned char byte3;
    } address;
};


static void checksum_payload(struct srts_payload *payload) {
    unsigned char *p = (unsigned char *) payload;
    unsigned char checksum = 0;
    int i = 0;

    for (i = 0; i < 7; i++) {
        checksum = checksum ^ p[i] ^ (p[i] >> 4);
    }
    checksum = checksum & 0xf;
    payload->checksum = checksum;
}

static void write_bit(int pin, char bit) {
    if (bit) {
        digitalWrite(pin, LOW);
        delayMicroseconds(660);
        digitalWrite(pin, HIGH);
        delayMicroseconds(660);
    } else {
        digitalWrite(pin, HIGH);
        delayMicroseconds(660);
        digitalWrite(pin, LOW);
        delayMicroseconds(660);
    }
}

static void write_byte(int pin, unsigned char byte) {
    unsigned int mask;

    for (mask = 0b10000000; mask != 0x0; mask >>= 1) {
        write_bit(pin, byte & mask);
    }
}

static void write_payload(int pin, struct srts_payload *payload) {
    unsigned char *p = (unsigned char *) payload;
    int i;

    for (i = 0; i < 7; i++) {
        write_byte(pin, p[i]);
    }
}

static void obfuscate_payload(struct srts_payload *payload) {
    unsigned char *p = (unsigned char *) payload;
    int i = 0;

    for (i = 1; i < 7; i++) {
        p[i] = p[i] ^ p[i - 1];
    }
}

static void sync_transmit(int pin, int repeated) {
    int count, i;

    if (repeated) {
        count = 7;
    } else {
        digitalWrite(pin, HIGH);
        delayMicroseconds(12400);
        digitalWrite(pin, LOW);
        for(i = 0; i != 5; i++) {
          delayMicroseconds(16000);
        }
        delayMicroseconds(600);
        count = 2;
    }
    for (i = 0; i != count; i++) {
        digitalWrite(pin, HIGH);
        delayMicroseconds(2560);
        digitalWrite(pin, LOW);
        delayMicroseconds(2560);
    }
}

void static write_interval_gap(int pin) {
  int i;

  digitalWrite(pin, LOW);
  for (i = 0; i != 2; i++) {
    delayMicroseconds(15000);
  }
  delayMicroseconds(400);
}

void srts_transmit(int pin, unsigned char key, unsigned short address,
        unsigned char command, unsigned short code, int repeated) {
    struct srts_payload payload;

    pinMode(pin, OUTPUT);
    sync_transmit(pin, repeated);

    digitalWrite(pin, HIGH);
    delayMicroseconds(4800);
    digitalWrite(pin, LOW);
    delayMicroseconds(660);

    payload.key = key;
    payload.ctrl = command;
    payload.checksum = 0;
    payload.code = code;
    payload.address.byte1 = ((char *) &address)[0];
    payload.address.byte2 = ((char *) &address)[1];
    payload.address.byte3 = 0;

    checksum_payload(&payload);
    obfuscate_payload(&payload);

    write_payload(pin, &payload);
    write_interval_gap(pin);
}


int pin = 10;

void setup(void) {
  Serial.begin(9600);

  randomSeed(analogRead(0));
}

void loop(void) {
  long key;
  int i;

  key = random(255);

  srts_transmit(pin, key, 3333, DOWN, 34, 0);

  for (i = 0; i != 7; i++) {
    srts_transmit(pin, key, 3333, DOWN, 34, 1);
  }
}
