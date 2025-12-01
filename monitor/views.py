from django.shortcuts import render, redirect
from django.contrib import messages
from django.contrib.auth.views import LoginView
from django.http import JsonResponse
from datetime import datetime, timedelta
from django.utils import timezone
from django.contrib.auth.decorators import login_required
import requests

from .models import WaterQualityRecord
from .forms import WaterQualityRecordForm

# ----- Login -----
class CustomLoginView(LoginView):
    template_name = 'monitor/login.html'

    def form_invalid(self, form):
        messages.error(self.request, "Invalid username or password.")
        return super().form_invalid(form)


# ----- Dashboard -----
@login_required
def dashboard(request):
    # Get filter date
    date_filter = request.GET.get('date_filter', 'today')
    custom_date = request.GET.get('custom_date')

    if date_filter == 'today':
        start_date = timezone.now().date()
    elif date_filter == 'yesterday':
        start_date = timezone.now().date() - timedelta(days=1)
    elif date_filter == 'custom' and custom_date:
        try:
            start_date = datetime.strptime(custom_date, "%Y-%m-%d").date()
        except ValueError:
            start_date = timezone.now().date()
    else:
        start_date = timezone.now().date()

    end_date = start_date + timedelta(days=1)

    # Filter records for selected date
    records = WaterQualityRecord.objects.filter(
        timestamp__gte=start_date,
        timestamp__lt=end_date
    ).order_by('timestamp')

    # Last record for sensor cards
    if records.exists():
        last_record = records.last()
        sensor_data = {
            'temperature': last_record.temperature,
            'tds': last_record.tds,
            'turbidity': last_record.turbidity,
        }
    else:
        sensor_data = {'temperature': 0, 'tds': 0, 'turbidity': 0}

    # Chart data
    chart_labels = [r.timestamp.strftime("%H:%M") for r in records]
    temp_data = [r.temperature for r in records]
    tds_data = [r.tds for r in records]
    turb_data = [r.turbidity for r in records]

    return render(request, 'monitor/dashboard.html', {
        'records': records,
        'sensor_data': sensor_data,
        'chart_labels': chart_labels,
        'temp_data': temp_data,
        'tds_data': tds_data,
        'turb_data': turb_data,
        'date_filter': date_filter,
        'custom_date': custom_date or ''
    })


# ----- Fetch Blynk data (can be called manually or by Celery) -----
def fetch_blynk_data():
    token = 'z1L-DQ3rYMi4zanyN9-P8VgGHJ1Xg7uO'
    base_url = 'https://blynk.cloud/external/api/get'

    pins = {
        'temperature': 'V2',
        'tds': 'V1',
        'turbidity': 'V0',
    }

    sensor_data = {}

    for label, pin in pins.items():
        try:
            url = f"{base_url}?token={token}&{pin}"
            response = requests.get(url)
            response.raise_for_status()
            value = response.text.strip()

            # convert to float if possible
            try:
                value = float(value)
            except:
                value = 0  # fallback if not numeric

            sensor_data[label] = value
        except Exception:
            sensor_data[label] = 0

    # Save to database
    WaterQualityRecord.objects.create(
        temperature=sensor_data.get('temperature', 0),
        tds=sensor_data.get('tds', 0),
        turbidity=sensor_data.get('turbidity', 0),
        recorded_by=None,  # system user
        source='Blynk',
        timestamp=timezone.now()
    )

    return sensor_data


# ----- API endpoint to trigger fetch manually -----
@login_required
def get_blynk_data(request):
    data = fetch_blynk_data()
    return JsonResponse(data)


# ----- Custom visuals page -----
@login_required
def custom_visuals(request):
    return render(request, 'monitor/custom_visuals.html')


# ----- Add manual record -----
@login_required
def add_record(request):
    if request.method == 'POST':
        form = WaterQualityRecordForm(request.POST)
        if form.is_valid():
            record = form.save(commit=False)
            record.recorded_by = request.user
            record.save()
            return redirect('dashboard')
    else:
        form = WaterQualityRecordForm()
    return render(request, 'monitor/add_record.html', {'form': form})
