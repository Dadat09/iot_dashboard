from django.db import models 
from django.contrib.auth.models import User

class WaterQualityRecord(models.Model):
     recorded_by = models.ForeignKey(User, on_delete=models.SET_NULL, null=True, blank=True)
     timestamp = models.DateTimeField(auto_now_add=False, help_text="Date and time of the record")
     temperature = models.FloatField(help_text="Water temperature in °C") 
     tds = models.FloatField(help_text="Total Dissolved Solids in ppm") 
     turbidity = models.FloatField(help_text="Turbidity in NTU")

     source = models.CharField( max_length=20, choices=[('Blynk', 'Blynk Sensor'), ('Manual', 'Manual Entry')], default='Blynk', help_text="Source of the data" )
     notes = models.TextField(blank=True, null=True, help_text="Optional notes or remarks")

     class Meta: ordering = ['-timestamp'] 
     def __str__(self): return f"{self.timestamp} | Temp: {self.temperature} °C | TDS: {self.tds} ppm | Turbidity: {self.turbidity} NTU"