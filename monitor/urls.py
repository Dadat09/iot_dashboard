from django.urls import path
from django.contrib.auth.views import LogoutView
from . import views

urlpatterns = [
    path('', views.dashboard, name='dashboard'),
    path('get-data/', views.get_blynk_data, name='get_blynk_data'),
    path('visuals/', views.custom_visuals, name='custom_visuals'),
    path('login/', views.CustomLoginView.as_view(), name='login'),
    path('logout/', LogoutView.as_view(next_page='login'), name='logout'),
    path('add-record/', views.add_record, name='add_record'),
]
