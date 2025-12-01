from django.contrib import admin
from .models import WaterQualityRecord

@admin.register(WaterQualityRecord)
class WaterQualityRecordAdmin(admin.ModelAdmin):
    list_display = ('timestamp', 'temperature', 'tds', 'turbidity', 'source', 'recorded_by')
    list_filter = ('source',)
    search_fields = ('temperature', 'tds', 'turbidity', 'notes')
