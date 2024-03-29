enum class SensorType {
	LM75=0,
	DHT11=1,
	DHT22=2,
	SCD40=3,
	SCD41=4
};
enum class ReadingType:uint32_t {
	TEMP=0,
	HUMID=1,
	CO2=2,
	PRES=3,
	REF_ALT=4,
	REF_OFFSET=5,
	SENSOR=6,
	SERIALNO=7
};
enum class ReadingUnit:uint32_t {
	CELSIUS=0,
	FAHRENHEIT=1,
	PERCENT=2,
	PPM=3,
	METERS=4,
	FEET=5,
	SENSOR=6,
	SERIALNO=7,
	HZ=8,
	DB=9
};
