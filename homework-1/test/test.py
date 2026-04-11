import pandas as pd
import plotly.graph_objects as go
import numpy as np


def trajectory_dataframe(wind_scale: float = 1.0) -> pd.DataFrame:
    """Тестовая траектория: сильнее ветер по Y → шире смещение (для разных вкладок)."""
    t = np.linspace(0, 5, 50)
    x = 25 * t - 0.2 * 2 * t**2
    y = wind_scale * 3 * t
    z = 100 - 4.9 * t**2
    df = pd.DataFrame({"t": t, "x": x, "y": y, "z": z})
    df = df[df["z"] >= 0].dropna()
    return df


def ground_mesh(df: pd.DataFrame):
    ground_x = np.linspace(0, df["x"].max() * 1.2, 10)
    ground_y = np.linspace(df["y"].min() - 10, df["y"].max() + 10, 10)
    gx, gy = np.meshgrid(ground_x, ground_y)
    gz = np.zeros_like(gx)
    return gx, gy, gz


def build_ballistics_3d_figure(df: pd.DataFrame, title: str) -> go.Figure:
    gx, gy, gz = ground_mesh(df)
    fig = go.Figure()

    fig.add_trace(
        go.Surface(
            x=gx,
            y=gy,
            z=gz,
            colorscale=[[0, "rgb(60,60,60)"], [1, "rgb(60,60,60)"]],
            showscale=False,
            opacity=0.9,
            name="Земля",
            hoverinfo="none",
        )
    )

    fig.add_trace(
        go.Scatter3d(
            x=df["x"],
            y=df["y"],
            z=df["z"],
            mode="lines+markers",
            line=dict(color=df["t"], colorscale="Plasma", width=8),
            marker=dict(size=4, opacity=0.9),
            name="Траектория сброса",
        )
    )

    df_labels = df.iloc[::10]
    fig.add_trace(
        go.Scatter3d(
            x=df_labels["x"],
            y=df_labels["y"],
            z=df_labels["z"],
            mode="text",
            text=df_labels["t"].apply(lambda val: f"{val:.1f}s"),
            textposition="top center",
            name="Время",
        )
    )

    y_pad = 10
    fig.update_layout(
        title=title,
        scene=dict(
            xaxis=dict(
                title="Дальность (X), м",
                gridcolor="gray",
                showbackground=True,
                backgroundcolor="rgb(230, 230, 230)",
            ),
            yaxis=dict(
                title="Смещение (Y), м",
                gridcolor="gray",
                showbackground=True,
                backgroundcolor="rgb(230, 230, 230)",
                range=[df["y"].min() - y_pad, df["y"].max() + y_pad],
            ),
            zaxis=dict(
                title="Высота (Z), м",
                gridcolor="gray",
                showbackground=True,
                backgroundcolor="rgb(200, 200, 200)",
                range=[0, df["z"].max() * 1.1],
            ),
            aspectmode="data",
        ),
        margin=dict(l=0, r=0, b=0, t=40),
    )
    return fig


# --- Сколько 3D-вкладок открыть: любой список (df, заголовок) или цикл по range ---
# Пример 1: явный список сценариев
runs = [
    (trajectory_dataframe(0.5), "Баллистика: слабый боковой ветер (×0.5)"),
    (trajectory_dataframe(1.0), "Баллистика: базовый ветер (×1)"),
    (trajectory_dataframe(2.0), "Баллистика: сильный боковой ветер (×2)"),
]

# Пример 2: столько фигур, сколько захотите — просто расширьте цикл / список:
# N = 5
# runs = [(trajectory_dataframe(0.2 * i), f"Вариант {i}") for i in range(1, N + 1)]

print(f"Открываю в браузере {len(runs)} вкладок с 3D…")
for df, plot_title in runs:
    build_ballistics_3d_figure(df, plot_title).show(renderer="browser")
