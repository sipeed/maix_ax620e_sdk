#include <timestamp.h>
#include <platform_def.h>

unsigned long timestamp_virt_addr = 0;
unsigned long timestamp_virt_tmp_addr = 0;
unsigned int timestamp_number = 0;

static u64 read_tmr64_val(void)
{
	return *((unsigned long*)TIMER64_CNT_LOW_ADDR);
}
static void store_timestamp(int subid, timestamp_data_t *data)
{
	u64 val;
	timestamp_header_t *header;
	header = (timestamp_header_t *)timestamp_virt_addr;
	val = read_tmr64_val();
	/*only save low 32 bit*/
	data->data[subid] = (val - header->wakeup_tmr64_cnt) & 0xffffffff;
}
unsigned int ax_sys_sleeptimestamp(int modid, unsigned int subid)
{
	timestamp_header_t *header;
	timestamp_data_t *data;
	timestamp_data_t *data_tmp;
	static unsigned int sleep_times = 0;
	unsigned long size = 0;
	int index = 0;

	if (modid != AX_ID_ATF)
		return -1;

	if((sleep_times == 0) && (subid == AX_SUB_ID_RESUME_START)) {
		size = sizeof(timestamp_data_t) + sizeof(timestamp_header_t);
		if (size > TIMESTAMP_STORE_IRAM_MAX_SIZE) {
			ERROR("size > atf max timestamp size.\r\n");
			return -1;
		}
		sleep_times = 1;
		timestamp_virt_addr = TIMESTAMP_START_STORE_IRAM_ADDR;
		memset((void*)timestamp_virt_addr, 0x0, size);
		timestamp_virt_tmp_addr = TIMESTAMP_START_STORE_IRAM_TMP_ADDR;
		memset((void*)timestamp_virt_tmp_addr, 0x0, size);
	}

	if (!timestamp_virt_addr || !timestamp_virt_tmp_addr|| subid >= MAX_SUBID)
		return -1;

	header = (timestamp_header_t *)timestamp_virt_addr;
	if(subid == AX_SUB_ID_RESUME_START) {
		/* copy data_tmp to data */
		data = (timestamp_data_t *)((void *)timestamp_virt_addr + sizeof(timestamp_header_t));
		data_tmp = (timestamp_data_t *)((void *)timestamp_virt_tmp_addr + sizeof(timestamp_header_t));
		for(index = 0; index < MAX_SUBID; index++)
			data->data[index] = data_tmp->data[index];
		header->wakeup_tmr64_cnt = read_tmr64_val();
		memset(data_tmp, 0, sizeof(timestamp_data_t));
	}
	data_tmp = (timestamp_data_t *)((void *)timestamp_virt_tmp_addr + sizeof(timestamp_header_t));
	store_timestamp(subid, data_tmp);
	return 0;
}