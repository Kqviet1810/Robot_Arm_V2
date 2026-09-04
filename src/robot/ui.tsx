import type { ReactNode } from "react";

export function Card({
  title,
  subtitle,
  right,
  children,
  className = "",
}: {
  title: string;
  subtitle?: string;
  right?: ReactNode;
  children: ReactNode;
  className?: string;
}) {
  return (
    <section className={`rounded-2xl border border-white/8 bg-white/[0.04] p-4 ${className}`}>
      <div className="mb-3 flex items-start justify-between gap-3">
        <div>
          <h3 className="text-[15px] font-semibold text-white">{title}</h3>
          {subtitle ? <p className="mt-1 text-xs text-white/55">{subtitle}</p> : null}
        </div>
        {right}
      </div>
      {children}
    </section>
  );
}
