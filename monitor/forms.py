from django import forms
from .models import WaterQualityRecord

class WaterQualityRecordForm(forms.ModelForm):
    class Meta:
        model = WaterQualityRecord
        fields = ['timestamp', 'temperature', 'tds', 'turbidity', 'source', 'notes']
        widgets = {
            'timestamp': forms.DateTimeInput(attrs={'type': 'datetime-local', 'class': 'form-control'}),
            'temperature': forms.NumberInput(attrs={'class': 'form-control', 'step': '0.1'}),
            'tds': forms.NumberInput(attrs={'class': 'form-control', 'step': '1'}),
            'turbidity': forms.NumberInput(attrs={'class': 'form-control', 'step': '0.1'}),
            'source': forms.Select(attrs={'class': 'form-select'}),
            'notes': forms.Textarea(attrs={'class': 'form-control', 'rows': 2}),
        }
