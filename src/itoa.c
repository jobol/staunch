/* 2014, Copyright Intel & Jose Bollo <jose.bollo@open.eurogiciel.org>, license MIT */

int _utoa(unsigned value, char *buffer)
{
	int r;
	char c, *p;

	p = buffer;
	while(value > 9) {
		*p++ = '0' + (char)(value % 10);
		value /= 10;
	}
	*p = '0' + (char)value;
	r = 1 + (p - buffer);
	while (p > buffer) {
		c = *buffer;
		*buffer++ = *p;
		*p-- = c;
	}
	
	return r;
}

void utoa(unsigned value, char *buffer)
{
	buffer[_utoa(value,buffer)] = 0;
}

int _itoa(int value, char *buffer)
{
	if (value < 0) {
		*buffer = '-';
		return _utoa((unsigned)-value, buffer+1) + 1;
	} else {
		return _utoa((unsigned)value, buffer);
	}
}

void itoa(int value, char *buffer)
{
	buffer[_itoa(value,buffer)] = 0;
}

