#include <ctype.h>
#include <math.h>

#include <Eina.h>
#include <GeoIP.h>

#define VIGRID_NUMBER_SEARCH "0123456789"

typedef struct _Community_Day Community_Day;
typedef struct _Community_Access Community_Access;
typedef struct _Community_Country Community_Country;
typedef struct _Community_IP Community_IP;

struct _Community_Day
{
   unsigned short year;
   unsigned char month;
   unsigned char day;

   Eina_Hash *hours;
};

struct _Community_IP
{
   const char *ip;
   const char *country;
};

struct _Community_Access
{
   Community_Day *day;
   Community_IP *ip;

   unsigned char hour;
   unsigned char minute;   
   unsigned char seconde;
};

struct _Community_Country
{
   const char *tld;
   const char *country;
   long long int count;
   long long int population;

   int popularity;

   Eina_Hash *access;
};

static Eina_Hash *community = NULL;
static Eina_Hash *days = NULL;
static Eina_Hash *countries = NULL;
static Eina_Hash *ips = NULL;

static long long int months[12];
static const char *MONTH_STRING[12] = {
  "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December"
};

static unsigned int
_community_day_key_length(const void *key EINA_UNUSED)
{
   return sizeof (Community_Day);
}

static int
_community_day_key_cmp(const void *key1, int key1_length EINA_UNUSED, const void *key2, int key2_length EINA_UNUSED)
{
   const Community_Day *d1 = key1;
   const Community_Day *d2 = key2;

   if (d1->year != d2->year) return d1->year - d2->year;
   if (d1->month != d2->month) return d1->month - d2->month;
   return d1->day - d2->day;
}

static int
_community_day_key_hash(const void *key, int key_length EINA_UNUSED)
{
   const Community_Day *d1 = key;
   unsigned int day;

   day = d1->day + d1->month * 31 + d1->year * 31 * 12;
   return eina_hash_int32(&day, 4);
}

static const char *
_eina_stringshare_up(const char *start, int length)
{
   char *tmp;
   char *r;
   int i = 0;

   r = alloca(length);
   for (tmp = r; i < length; ++i, ++tmp, ++start)
     *tmp = toupper(*start);
   *tmp = 0;

   return eina_stringshare_add(r);
}

static long long int
_eina_file_int_get(Eina_File_Line *line, unsigned long long *offset)
{
   long long int n = 0;
   const char *s;

   for (s = line->start + *offset; s < line->end; s++)
     {
        const char *convert = VIGRID_NUMBER_SEARCH;
        const char *r;

        r = strchr(convert, *s);
        if (!r)
          {
             *offset = s - line->start;
             return n;
          }
        n = n * 10 + (r - convert);
     }

   *offset = line->length;
   return n;
}

static const char *
_eina_file_filter_get(Eina_File_Line *line, unsigned long long *offset, const char *filter)
{
   unsigned long long start = *offset;
   const char *s;

   for (s = line->start + *offset; s < line->end; s++)
     {
        const char *r;

        r = strchr(filter, *s);
        if (!r) break ;
     }

   *offset = s - line->start;

   return eina_stringshare_add_length(line->start + start, *offset - start);
}

static const char *
_eina_file_ip_get(Eina_File_Line *line, unsigned long long *offset)
{
   return _eina_file_filter_get(line, offset, VIGRID_NUMBER_SEARCH".:");
}

int
main(int argc, char **argv)
{
   Eina_File_Direct_Info *info;
   Eina_Iterator *it;
   Eina_File_Line *l;
   Eina_File *f;
   GeoIP *geo;
   Community_Country *country;
   FILE *log;
   int i;

   if (argc != 2) return -1;

   eina_init();

   community = eina_hash_stringshared_new(NULL);
   ips = eina_hash_stringshared_new(NULL);
   days = eina_hash_new(_community_day_key_length,
			_community_day_key_cmp,
			_community_day_key_hash,
			NULL, 5);
   countries = eina_hash_string_superfast_new(NULL);
   geo = GeoIP_new(GEOIP_STANDARD);
   memset(months, 0, sizeof (months));

   /* Read real name country to tld file */
   f = eina_file_open("country_tld.csv", EINA_FALSE);
   if (!f) return -1;

   it = eina_file_map_lines(f);
   EINA_ITERATOR_FOREACH(it, l)
     {
        const char *s;

        s = memchr(l->start, ',', l->length);
        if (!s) continue ;

        country = calloc(1, sizeof (Community_Country));
        country->tld = _eina_stringshare_up(l->start, s - l->start);
        country->country = _eina_stringshare_up(s + 1, l->length - (s - l->start + 1));
        country->access = eina_hash_stringshared_new(NULL);

        eina_hash_direct_add(countries, country->tld, country);
     }
   eina_iterator_free(it);
   eina_file_close(f);

   /* Read population information */
   f = eina_file_open("country_population.csv", EINA_FALSE);
   if (!f) return -1;

   it = eina_file_map_lines(f);
   EINA_ITERATOR_FOREACH(it, l)
     {
        Eina_Iterator *it2;
        const char *s;
        const char *country_name;
        const char *r = NULL;

        for (s = l->start; s < l->end; s++)
          {
             const char *convert = VIGRID_NUMBER_SEARCH;

             r = strchr(convert, *s);
             if (r) break ;
          }

        if (!r) continue;

        country = NULL;
        country_name = _eina_stringshare_up(l->start, s - l->start - 1);

        it2 = eina_hash_iterator_data_new(countries);
        EINA_ITERATOR_FOREACH(it2, country)
          {
             if (country->country == country_name)
               {
                  unsigned long long offset = s - l->start;

                  country->population = _eina_file_int_get(l, &offset);
                  break;
               }
          }
        eina_iterator_free(it2);
     }
   eina_iterator_free(it);
   eina_file_close(f);

   /* Read logs directory */
   it = eina_file_stat_ls(argv[1]);
   EINA_ITERATOR_FOREACH(it, info)
     {
        Eina_Iterator *it2;
        Eina_File *f;

        if (info->type != EINA_FILE_REG) continue ;

        f = eina_file_open(info->path, EINA_FALSE);
        if (!f) continue ;

        fprintf(stderr, "processing: %s\n", info->path);
        it2 = eina_file_map_lines(f);
        EINA_ITERATOR_FOREACH(it2, l)
          {
             Community_Country *country;
             Community_IP *cip;
             Community_Day *cday;
             Community_Day tday;
             const char *ip;
             unsigned long long offset = 0;
             long long int year, month, day, hour, min, sec;

             year = _eina_file_int_get(l, &offset);
             if (offset == l->length || l->start[offset] != '-') continue ;
             offset++;

             month = _eina_file_int_get(l, &offset);
             if (offset == l->length || l->start[offset] != '-') continue ;
             offset++;

             day = _eina_file_int_get(l, &offset);
             if (offset == l->length || l->start[offset] != 'T') continue ;
             offset++;

             hour = _eina_file_int_get(l, &offset);
             if (offset == l->length || l->start[offset] != ':') continue ;
             offset++;

             min = _eina_file_int_get(l, &offset);
             if (offset == l->length || l->start[offset] != ':') continue ;
             offset++;

             sec = _eina_file_int_get(l, &offset);
             if (offset == l->length || l->start[offset] != '-') continue ;
             offset++;

             /* discarding some useless information aka time zone */
             _eina_file_int_get(l, &offset);
             if (offset == l->length || l->start[offset] != ':') continue ;
             offset++;

             _eina_file_int_get(l, &offset);
             if (offset == l->length || l->start[offset] != '/') continue ;
             offset++;

             _eina_file_int_get(l, &offset);
             if (offset == l->length || l->start[offset] != ' ') continue ;
             offset++;

             /* Now forget the server */
             while (offset < l->length && l->start[offset] != ' ')
               offset++;
             if (offset == l->length || l->start[offset] != ' ') continue ;
             offset++;

             /* Server IP adress */
             _eina_file_ip_get(l, &offset);
             if (offset == l->length || l->start[offset] != ' ') continue ;

             /* Service served */
             while (offset < l->length && l->start[offset] != ' ')
               offset++;
             if (offset == l->length || l->start[offset] != ' ') continue ;
             offset++;

             /* Get client IP */
             ip = _eina_file_ip_get(l, &offset);
             if (offset == l->length || l->start[offset] != ' ') continue ;

             if (year < 2012 || year > 2012 ||
                 month < 0 || month > 12 ||
                  day < 0 || day > 31 ||  hour < 0 || hour > 24 ||
                    min < 0 || min > 60 ||
                 sec < 0 || sec > 60)
               continue ;

             tday.year = year;
             tday.month = month;
             tday.day = day;
             tday.hours = NULL;

             cday = eina_hash_find(days, &tday);
             if (!cday)
               {
                  cday = calloc(1, sizeof (Community_Day));
                  *cday = tday;
                  cday->hours = eina_hash_int32_new(NULL);
                  eina_hash_add(days, &tday, cday);
               }

             cip = eina_hash_find(ips, ip);
             if (!cip)
               {
                  cip = calloc(1, sizeof (Community_IP));
                  cip->ip = eina_stringshare_ref(ip);
                  cip->country = GeoIP_country_code_by_addr(geo, ip);

                  if (!cip->country)
                    {
                       fprintf(stderr, "unknown ip address location: '%s'\n", ip);
                       cip->country = "??";
                    }

                  eina_hash_direct_add(ips, cip->ip, cip);
               }

             country = eina_hash_find(countries, cip->country);
             if (!country)
               {
                  fprintf(stderr, "TLD: '%s' not found\n", cip->country);
                  country = calloc(1, sizeof (Community_Country));
                  country->tld = cip->country;
                  country->access = eina_hash_stringshared_new(NULL);

                  eina_hash_direct_add(countries, country->tld, country);
               }
          }
        eina_iterator_free(it2);
        eina_file_close(f);
     }
   eina_iterator_free(it);

   return 0;
}
